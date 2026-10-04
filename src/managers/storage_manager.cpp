#include "managers/storage_manager.h"
#include <FS.h>
#include <vfs_api.h>
#include <driver/sdmmc_host.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <diskio_sdmmc.h>
#include <algorithm>
#include "app_config.h"

StorageManager storage_manager;

// Mounted with the IDF API directly (not SD_MMC) so we keep the card handle and can poll it:
// with no card-detect pin, a CMD13 status query is how a removed card is noticed.
static const char *MOUNT_POINT = "/sdcard";
static const char *FOLDERS[] = SD_FOLDERS;

static std::shared_ptr<VFSImpl> s_vfs = std::make_shared<VFSImpl>();
static fs::FS s_fs(s_vfs);
static sdmmc_card_t *s_card;

static SemaphoreHandle_t s_lock;       // held by file operations and by unmount
static volatile bool s_mounted;
static void (*s_before_unmount[4])();  // lets long-lived file users (audio) close their files first
static int s_hook_count;
static volatile bool s_need_remount;   // set after a failed operation
static volatile uint32_t s_mount_count;
static char s_status[40] = "No card";
static volatile float s_free_gb, s_total_gb;

static void update_status() {
  if (!s_mounted) {
    strlcpy(s_status, "No card", sizeof(s_status));
    s_free_gb = s_total_gb = 0;
    return;
  }
  char drive[3] = {(char)('0' + ff_diskio_get_pdrv_card(s_card)), ':', 0};
  FATFS *fs;
  DWORD free_clusters;
  if (f_getfree(drive, &free_clusters, &fs) != FR_OK) return;  // slow on big cards the first time: task only
  const double gb = 1024.0 * 1024.0 * 1024.0;
  const double cluster = (double)fs->csize * 512;
  s_free_gb = free_clusters * cluster / gb;
  s_total_gb = (fs->n_fatent - 2) * cluster / gb;
  snprintf(s_status, sizeof(s_status), "%.1f GB free of %.1f GB", (float)s_free_gb, (float)s_total_gb);
}

static void unmount() {
  if (!s_card) return;
  esp_vfs_fat_sdcard_unmount(MOUNT_POINT, s_card);
  s_card = nullptr;
}

static bool mount() {
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.max_freq_khz = SD_FREQ_KHZ;
  sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
  slot.width = 4;
  slot.clk = (gpio_num_t)SD_PIN_CLK;
  slot.cmd = (gpio_num_t)SD_PIN_CMD;
  slot.d0 = (gpio_num_t)SD_PIN_D0;
  slot.d1 = (gpio_num_t)SD_PIN_D1;
  slot.d2 = (gpio_num_t)SD_PIN_D2;
  slot.d3 = (gpio_num_t)SD_PIN_D3;
  esp_vfs_fat_mount_config_t cfg = {};
  cfg.format_if_mount_failed = SD_FORMAT_IF_MOUNT_FAILED;
  cfg.max_files = SD_MAX_OPEN_FILES;
  cfg.allocation_unit_size = 16 * 1024;

  if (esp_vfs_fat_sdmmc_mount(MOUNT_POINT, &host, &slot, &cfg, &s_card) != ESP_OK) {
    s_card = nullptr;
    return false;
  }
  for (const char *dir : FOLDERS) {
    if (!s_fs.exists(dir) && !s_fs.mkdir(dir)) {
      unmount();
      return false;
    }
  }
  return true;
}

static void mount_task(void *) {
  uint32_t last_try = millis() - SD_RETRY_MS, last_status = 0;  // first mount attempt right away
  for (;;) {
    // A failed read/write, or the card pulled (it stops answering CMD13)
    if (s_need_remount || (s_mounted && sdmmc_get_status(s_card) != ESP_OK)) {
      s_mounted = false;  // no new operations from here on
      for (int i = 0; i < s_hook_count; i++) s_before_unmount[i]();
      xSemaphoreTake(s_lock, portMAX_DELAY);  // no file operation in progress
      s_need_remount = false;
      unmount();
      Serial.println("SD card removed or failed");
      xSemaphoreGive(s_lock);
      update_status();
      last_try = millis();
    }
    if (!s_mounted && millis() - last_try >= SD_RETRY_MS) {
      last_try = millis();
      if (mount()) {
        s_mounted = true;
        s_mount_count = s_mount_count + 1;
        update_status();
        Serial.printf("SD card mounted: %s\n", s_status);
        last_status = millis();
      }
    } else if (s_mounted && millis() - last_status > SD_STATUS_REFRESH_MS) {
      update_status();
      last_status = millis();
    }
    vTaskDelay(pdMS_TO_TICKS(SD_CHECK_MS));
  }
}

void StorageManager::begin() {
  s_lock = xSemaphoreCreateMutex();
  s_vfs->mountpoint(MOUNT_POINT);
  xTaskCreatePinnedToCore(mount_task, "sd_mount", 4096, nullptr, 1, nullptr, 0);
}

bool StorageManager::mounted() const { return s_mounted; }
uint32_t StorageManager::mount_count() const { return s_mount_count; }
const char *StorageManager::status_text() const { return s_status; }
float StorageManager::free_gb() const { return s_free_gb; }
float StorageManager::total_gb() const { return s_total_gb; }

// An open / write failed. Only a card that stops answering is worth remounting for: an open also fails
// when all SD_MAX_OPEN_FILES are in use, and remounting then would drop the sound that is playing.
void StorageManager::failed() {
  if (!s_mounted || !s_card || sdmmc_get_status(s_card) == ESP_OK) return;
  s_mounted = false;
  s_need_remount = true;  // the task unmounts, updates the status and retries
}

// Holds the lock for one file operation; ok() is false when there is no card
class Guard {
 public:
  Guard() { xSemaphoreTake(s_lock, portMAX_DELAY); }
  ~Guard() { xSemaphoreGive(s_lock); }
  bool ok() const { return s_mounted; }
};

bool StorageManager::exists(const char *path) {
  Guard g;
  return g.ok() && s_fs.exists(path);
}

bool StorageManager::read_file(const char *path, String &out) {
  Guard g;
  if (!g.ok() || !s_fs.exists(path)) return false;  // a missing file is not a card failure
  File f = s_fs.open(path, FILE_READ);
  if (!f) return false;
  out = f.readString();
  f.close();
  return true;
}

bool StorageManager::write_file(const char *path, const String &data) {
  Guard g;
  if (!g.ok()) return false;
  const String tmp = String(path) + ".tmp";
  File f = s_fs.open(tmp.c_str(), FILE_WRITE);
  if (!f) {
    failed();
    return false;
  }
  const bool ok = f.print(data) == data.length();
  f.close();
  if (!ok) {
    failed();
    return false;
  }
  if (s_fs.exists(path)) s_fs.remove(path);
  if (!s_fs.rename(tmp.c_str(), path)) {
    failed();
    return false;
  }
  return true;
}

bool StorageManager::append_line(const char *path, const String &line) {
  Guard g;
  if (!g.ok()) return false;
  File f = s_fs.open(path, FILE_APPEND);
  if (!f) {
    failed();
    return false;
  }
  const bool ok = f.print(line) == line.length() && f.print('\n') == 1;
  f.close();
  if (!ok) failed();
  return ok;
}

bool StorageManager::list(const char *dir, std::vector<String> &names) {
  names.clear();
  Guard g;
  if (!g.ok()) return false;
  File d = s_fs.open(dir);
  if (!d || !d.isDirectory()) return false;
  for (File f = d.openNextFile(); f; f = d.openNextFile()) {
    if (!f.isDirectory()) names.push_back(f.name());
  }
  std::sort(names.begin(), names.end());
  return true;
}

long StorageManager::file_size(const char *path) {
  Guard g;
  if (!g.ok() || !s_fs.exists(path)) return -1;
  File f = s_fs.open(path, FILE_READ);
  const long size = f ? (long)f.size() : -1;
  f.close();
  return size;
}

bool StorageManager::rename(const char *from, const char *to) {
  Guard g;
  if (!g.ok()) return false;
  if (s_fs.exists(to)) s_fs.remove(to);
  return s_fs.rename(from, to);
}

bool StorageManager::remove(const char *path) {
  Guard g;
  return g.ok() && s_fs.remove(path);
}

fs::FS &StorageManager::fs() { return s_fs; }

void StorageManager::add_before_unmount(void (*hook)()) {
  if (s_hook_count < 4) s_before_unmount[s_hook_count++] = hook;
}
