#pragma once
#include <Arduino.h>
#include <FS.h>
#include <vector>

// microSD card over 4-bit SDMMC. A background task mounts the card (and retries every SD_RETRY_MS
// while there is none), creates SD_FOLDERS, and keeps the free-space figure current, so a missing
// card never blocks loop(). There is no card-detect pin: the task polls the card every SD_CHECK_MS
// (CMD13) and unmounts it when it stops answering or a read/write fails.
//
// Paths are card paths ("/audio/chime.mp3"). Every call is safe with no card (it just returns false).
class StorageManager {
 public:
  void begin();

  bool mounted() const;
  uint32_t mount_count() const;       // increments on every successful mount
  const char *status_text() const;    // "No card", "12.3 GB free of 29.7 GB", ...
  float free_gb() const;              // 0 with no card (refreshed every SD_STATUS_REFRESH_MS)
  float total_gb() const;

  bool exists(const char *path);
  bool read_file(const char *path, String &out);
  bool write_file(const char *path, const String &data);  // temp file + rename, never half-written
  bool append_line(const char *path, const String &line); // logs and history (adds '\n')
  bool remove(const char *path);
  long file_size(const char *path);  // -1 = missing / no card
  bool rename(const char *from, const char *to);
  bool list(const char *dir, std::vector<String> &names);  // file names (no folders) in dir, sorted

  // For code that keeps a file open across loop() calls (audio playback): the card's file system, and
  // hooks (up to 4) the mount task calls (from its own task) before unmounting, which must close those files.
  fs::FS &fs();
  void add_before_unmount(void (*hook)());

 private:
  void failed();  // an operation failed: assume the card is gone and let the task remount
};

extern StorageManager storage_manager;
