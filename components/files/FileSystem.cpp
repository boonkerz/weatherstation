#include "FileSystem.h"

static char tag[] = "FileSystem";

void FileSystem::init() {

	if (isRegistered) return;

	esp_vfs_t vfs;
	esp_err_t err;

	vfs.flags = ESP_VFS_FLAG_CONTEXT_PTR;
	vfs.write_p  = vfs_write;
	vfs.lseek_p  = vfs_lseek;
	vfs.read_p   = vfs_read;
	vfs.open_p   = vfs_open;
	vfs.close_p  = vfs_close;
	vfs.fstat_p  = vfs_fstat;
	vfs.stat_p   = vfs_stat;
	vfs.link_p   = vfs_link;
	vfs.unlink_p  = vfs_unlink;
	vfs.rename_p = vfs_rename;

	err = esp_vfs_register("/spiffs", &vfs, NULL);
	if (err != ESP_OK) {
		ESP_LOGE(tag, "esp_vfs_register: err=%d", err);
	}
	isRegistered = true;

	mount();

}

void FileSystem::mount() {
	if (!isRegistered) return;
	if (isMounted) return;

	// spiffs nucleus file descriptor
	typedef struct {
	  // the filesystem of this descriptor
	  spiffs *fs;
	  // number of file descriptor - if 0, the file descriptor is closed
	  spiffs_file file_nbr;
	  // object id - if SPIFFS_OBJ_ID_ERASED, the file was deleted
	  spiffs_obj_id obj_id;
	  // size of the file
	  u32_t size;
	  // cached object index header page index
	  spiffs_page_ix objix_hdr_pix;
	  // cached offset object index page index
	  spiffs_page_ix cursor_objix_pix;
	  // cached offset object index span index
	  spiffs_span_ix cursor_objix_spix;
	  // current absolute offset
	  u32_t offset;
	  // current file descriptor offset
	  u32_t fdoffset;
	  // fd flags
	  spiffs_flags flags;
	#if SPIFFS_IX_MAP
	  // spiffs index map, if 0 it means unmapped
	  spiffs_ix_map *ix_map;
	#endif
	} spiffs_fd;

	spiffs_config cfg;
	int res = 0;
	int retries = 0;
	int err = 0;

	ESP_LOGI(tag, "Mounting SPIFFS files system");

	cfg.phys_addr 		 = CONFIG_SPIFFS_BASE_ADDR;
	cfg.phys_size 		 = CONFIG_SPIFFS_SIZE;
	cfg.phys_erase_block = 4096;
	cfg.log_page_size    = CONFIG_SPIFFS_LOG_PAGE_SIZE;
	cfg.log_block_size   = CONFIG_SPIFFS_LOG_BLOCK_SIZE;

	cfg.hal_read_f  = (spiffs_read)esp32_spi_flash_read;
	cfg.hal_write_f = (spiffs_write)esp32_spi_flash_write;
	cfg.hal_erase_f = (spiffs_erase)esp32_spi_flash_erase;

	my_spiffs_work_buf = (u8_t*)malloc(cfg.log_page_size * 8);
	if (!my_spiffs_work_buf) {
		err = 1;
		return;
	}

	int fds_len = sizeof(spiffs_fd) * SPIFFS_TEMPORAL_CACHE_HIT_SCORE;
	my_spiffs_fds = (u8_t*)malloc(fds_len);
	if (!my_spiffs_fds) {
		free(my_spiffs_work_buf);
		err = 2;
		return;
	}

	int cache_len = cfg.log_page_size * SPIFFS_TEMPORAL_CACHE_HIT_SCORE;
	my_spiffs_cache = (u8_t*)malloc(cache_len);
	if (!my_spiffs_cache) {
		free(my_spiffs_work_buf);
		free(my_spiffs_fds);
		err = 3;
		return;
	}

	ESP_LOGI(tag, "Start address: 0x%x; Size %d KB", cfg.phys_addr, cfg.phys_size / 1024);
	ESP_LOGI(tag, "  Work buffer: %d B", cfg.log_page_size * 8);
	ESP_LOGI(tag, "   FDS buffer: %d B", sizeof(spiffs_fd) * SPIFFS_TEMPORAL_CACHE_HIT_SCORE);
	ESP_LOGI(tag, "   Cache size: %d B", cfg.log_page_size * SPIFFS_TEMPORAL_CACHE_HIT_SCORE);
	while (retries < 2) {
		res = SPIFFS_mount(
				&fs, &cfg, my_spiffs_work_buf, my_spiffs_fds,
				fds_len, my_spiffs_cache, cache_len, NULL
		);

		if (res < 0) {
			if (fs.err_code == SPIFFS_ERR_NOT_A_FS) {
				ESP_LOGW(tag, "No file system detected, formating...");
				SPIFFS_unmount(&fs);
				res = SPIFFS_format(&fs);
				if (res < 0) {
					free(my_spiffs_work_buf);
					free(my_spiffs_fds);
					free(my_spiffs_cache);
					ESP_LOGE(tag, "Format error");
					return;
				}
			}
			else {
				free(my_spiffs_work_buf);
				free(my_spiffs_fds);
				free(my_spiffs_cache);
				ESP_LOGE(tag, "Error mounting fs (%d)", res);
				return;
			}
		}
		else break;
		retries++;
	}

	if (retries > 1) {
		free(my_spiffs_work_buf);
		free(my_spiffs_fds);
		free(my_spiffs_cache);
		ESP_LOGE(tag, "Can't mount");
		return;
	}

	//list_init(&files, 0);

	ESP_LOGI(tag, "Mounted");

	isMounted = true;
}


bool FileSystem::fileExists(std::string name) {

	struct stat st;
	if (stat(("/spiffs/" + name).c_str(), &st) == 0) {
		return true;
	}

	return false;
}

string FileSystem::spiffsErrorToString(int code) {
	static char msg[10];
	switch(code) {
		case SPIFFS_OK:
			return "SPIFFS_OK";
		case SPIFFS_ERR_NOT_MOUNTED:
			return "SPIFFS_ERR_NOT_MOUNTED";
		case SPIFFS_ERR_FULL:
			return "SPIFFS_ERR_FULL";
		case SPIFFS_ERR_NOT_FOUND:
			return "SPIFFS_ERR_NOT_FOUND";
		case SPIFFS_ERR_END_OF_OBJECT:
			return "SPIFFS_ERR_END_OF_OBJECT";
		case SPIFFS_ERR_DELETED:
			return "SPIFFS_ERR_DELETED";
		case SPIFFS_ERR_FILE_CLOSED:
			return "SPIFFS_ERR_FILE_CLOSED";
		case SPIFFS_ERR_FILE_DELETED:
			return "SPIFFS_ERR_FILE_DELETED";
		case SPIFFS_ERR_BAD_DESCRIPTOR:
			return "SPIFFS_ERR_BAD_DESCRIPTOR";
		case SPIFFS_ERR_NOT_A_FS:
			return "SPIFFS_ERR_NOT_A_FS";
		case SPIFFS_ERR_FILE_EXISTS:
			return "SPIFFS_ERR_FILE_EXISTS";
	}
	sprintf(msg, "%d", code);
	return msg;
}

void FileSystem::logFlags(int flags) {
	ESP_LOGD(tag, "flags:");
	if (flags & O_APPEND) {
		ESP_LOGD(tag, "- O_APPEND");
	}
	if (flags & O_CREAT) {
		ESP_LOGD(tag, "- O_CREAT");
	}
	if (flags & O_TRUNC) {
		ESP_LOGD(tag, "- O_TRUNC");
	}
	if (flags & O_RDONLY) {
		ESP_LOGD(tag, "- O_RDONLY");
	}
	if (flags & O_WRONLY) {
		ESP_LOGD(tag, "- O_WRONLY");
	}
	if (flags & O_RDWR) {
		ESP_LOGD(tag, "- O_RDWR");
	}
} // End of logFlags


ssize_t FileSystem::vfs_write(void *ctx, int fd, const void *data, size_t size) {
	ESP_LOGI(tag, ">> write fd=%d, data=0x%lx, size=%d", fd, (unsigned long)data, size);
	spiffs *fs = (spiffs *)ctx;
	ssize_t retSize = SPIFFS_write(fs, (spiffs_file)fd, (void *)data, size);
	return retSize;
} // vfs_write


off_t FileSystem::vfs_lseek(void *ctx, int fd, off_t offset, int whence) {
	ESP_LOGI(tag, ">> lseek fd=%d, offset=%d, whence=%d", fd, (int)offset, whence);
	return 0;
} // vfs_lseek


ssize_t FileSystem::vfs_read(void *ctx, int fd, void *dst, size_t size) {
	ESP_LOGI(tag, ">> read fd=%d, dst=0x%lx, size=%d", fd, (unsigned long)dst, size);
	spiffs *fs = (spiffs *)ctx;
	ssize_t retSize = SPIFFS_read(fs, (spiffs_file)fd, dst, size);
	return retSize;
} // vfs_read

int FileSystem::vfs_open(void *ctx, const char *path, int flags, int accessMode) {
	ESP_LOGI(tag, ">> open path=%s, flags=0x%x, accessMode=0x%x", path, flags, accessMode);
	logFlags(flags);
	spiffs *fs = (spiffs *)ctx;
	int spiffsFlags = 0;
	if (flags & O_CREAT) {
		spiffsFlags |= SPIFFS_O_CREAT;
	}
	if (flags & O_TRUNC) {
		spiffsFlags |= SPIFFS_O_TRUNC;
	}
	if (flags & O_RDONLY) {
		spiffsFlags |= SPIFFS_O_RDONLY;
	}
	if (flags & O_WRONLY) {
		spiffsFlags |= SPIFFS_O_WRONLY;
	}
	if (flags & O_RDWR) {
		spiffsFlags |= SPIFFS_O_RDWR;
	}
	if (flags & O_APPEND) {
		spiffsFlags |= SPIFFS_O_APPEND;
	}
	int rc = SPIFFS_open(fs, path, spiffsFlags, accessMode);
	return rc;
} // vfs_open


int FileSystem::vfs_close(void *ctx, int fd) {
	ESP_LOGI(tag, ">> close fd=%d", fd);
	spiffs *fs = (spiffs *)ctx;
	int rc = SPIFFS_close(fs, (spiffs_file)fd);
	return rc;
} // vfs_close


int FileSystem::vfs_fstat(void *ctx, int fd, struct stat *st) {
	ESP_LOGI(tag, ">> fstat fd=%d", fd);
	return 0;
} // vfs_fstat


int FileSystem::vfs_stat(void *ctx, const char *path, struct stat *st) {
	ESP_LOGI(tag, ">> stat path=%s", path);
	return 0;
} // vfs_stat


int FileSystem::vfs_link(void *ctx, const char *oldPath, const char *newPath) {
	ESP_LOGI(tag, ">> link oldPath=%s, newPath=%s", oldPath, newPath);
	return 0;
} // vfs_link

int FileSystem::vfs_unlink(void *ctx, const char *path) {
	ESP_LOGI(tag, ">> unlink path=%s", path);
	spiffs *fs = (spiffs *)ctx;
	SPIFFS_remove(fs, path);
	return 0;
} // vfs_unlink


int FileSystem::vfs_rename(void *ctx, const char *oldPath, const char *newPath) {
	ESP_LOGI(tag, ">> rename oldPath=%s, newPath=%s", oldPath, newPath);
	spiffs *fs = (spiffs *)ctx;
	int rc = SPIFFS_rename(fs, oldPath, newPath);
	return rc;
} // vfs_rename



s32_t FileSystem::esp32_spi_flash_read(u32_t addr, u32_t size, u8_t *dst) {
	u32_t aaddr;
	u8_t *buff = NULL;
	u8_t *abuff = NULL;
	u32_t asize;

	asize = size;

	// Align address to 4 byte
	aaddr = (addr + (4 - 1)) & (u32_t)-4;
	if (aaddr != addr) {
		aaddr -= 4;
		asize += (addr - aaddr);
	}

	// Align size to 4 byte
	asize = (asize + (4 - 1)) & (u32_t)-4;

	if ((aaddr != addr) || (asize != size)) {
		// Align buffer
		buff = (u8_t*)malloc(asize + 4);
		if (!buff) {
			return SPIFFS_ERR_INTERNAL;
		}

		abuff = (u8_t *)(((ptrdiff_t)buff + (4 - 1)) & (u32_t)-4);

		if (spi_flash_read(aaddr, (void *)abuff, asize) != 0) {
			free(buff);
			return SPIFFS_ERR_INTERNAL;
		}

		memcpy(dst, abuff + (addr - aaddr), size);

		free(buff);
	} else {
		if (spi_flash_read(addr, (void *)dst, size) != 0) {
			return SPIFFS_ERR_INTERNAL;
		}
	}

    return SPIFFS_OK;
}

s32_t FileSystem::esp32_spi_flash_write(u32_t addr, u32_t size, const u8_t *src) {
	u32_t aaddr;
	u8_t *buff = NULL;
	u8_t *abuff = NULL;
	u32_t asize;

	asize = size;

	// Align address to 4 byte
	aaddr = (addr + (4 - 1)) & -4;
	if (aaddr != addr) {
		aaddr -= 4;
		asize += (addr - aaddr);
	}

	// Align size to 4 byte
	asize = (asize + (4 - 1)) & -4;

	if ((aaddr != addr) || (asize != size)) {
		// Align buffer
		buff = (u8_t*)malloc(asize + 4);
		if (!buff) {
			return SPIFFS_ERR_INTERNAL;
		}

		abuff = (u8_t *)(((ptrdiff_t)buff + (4 - 1)) & -4);

		if (spi_flash_read(aaddr, (void *)abuff, asize) != 0) {
			free(buff);
			return SPIFFS_ERR_INTERNAL;
		}

		memcpy(abuff + (addr - aaddr), src, size);

		if (spi_flash_write(aaddr, (uint32_t *)abuff, asize) != 0) {
			free(buff);
			return SPIFFS_ERR_INTERNAL;
		}

		free(buff);
	} else {
		if (spi_flash_write(addr, (uint32_t *)src, size) != 0) {
			return SPIFFS_ERR_INTERNAL;
		}
	}

    return SPIFFS_OK;
}

s32_t FileSystem::esp32_spi_flash_erase(u32_t addr, u32_t size) {
	if (spi_flash_erase_sector(addr >> 12) != 0) {
		return SPIFFS_ERR_INTERNAL;
	}

    return SPIFFS_OK;
}
