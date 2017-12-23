#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "esp_err.h"
#include "esp_log.h"
#include "spiffs_esp.h"
#include <fcntl.h>
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include <esp_spi_flash.h>

using namespace std;

class FileSystem {
private:
	u8_t *my_spiffs_work_buf;
	u8_t *my_spiffs_fds;
	u8_t *my_spiffs_cache;
	spiffs fs;
	bool isMounted = false;
	bool isRegistered = false;
	void mount();
	static void logFlags(int flags);
	string spiffsErrorToString(int code);
	static ssize_t vfs_write(void *ctx, int fd, const void *data, size_t size);
	static ssize_t vfs_read(void *ctx, int fd, void *dst, size_t size);
	static off_t vfs_lseek(void *ctx, int fd, off_t offset, int whence);
	static int vfs_rename(void *ctx, const char *oldPath, const char *newPath);
	static int vfs_unlink(void *ctx, const char *path);
	static int vfs_link(void *ctx, const char *oldPath, const char *newPath);
	static int vfs_stat(void *ctx, const char *path, struct stat *st);
	static int vfs_fstat(void *ctx, int fd, struct stat *st);
	static int vfs_close(void *ctx, int fd);
	static int vfs_open(void *ctx, const char *path, int flags, int accessMode);
	static s32_t esp32_spi_flash_read(u32_t addr, u32_t size, u8_t *dst);
	static s32_t esp32_spi_flash_write(u32_t addr, u32_t size, const u8_t *src);
	static s32_t esp32_spi_flash_erase(u32_t addr, u32_t size);
public:
	void init();
	bool fileExists(std::string name);
};
