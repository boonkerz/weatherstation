#include "FileSystem.h"

static char tag[] = "FileSystem";

void FileSystem::init() {

}

int FileSystem::read(string path, uint8_t *buf, int &length) {


	FILE *fd = fopen(path.c_str(), "rb");
	if (fd == NULL) {
		ESP_LOGE(tag, ">> fopen failed");
		return -1;
	}
	int res = fread(buf, 1, 3800, fd);
	if (res <= 0) {
		ESP_LOGE(tag, ">> fread failed: %d", res);
		res = fclose(fd);
		if (res) {
			ESP_LOGE(tag, ">> fclose failed: %d", res);
			return -2;
		}
		return -3;
	}
	ESP_LOGI(tag, ">> %d bytes read \r\n", res);
	length = res;
	res = fclose(fd);
	if (res) {
		ESP_LOGE(tag, ">> fclose failed: %d", res);
		return -4;
	}

	return 0;
}


void FileSystem::ls(string path) {



	ESP_LOGD(tag, ">> \r\nListing folder %s\r\n", path.c_str());

	// open the specified folder
	DIR *dir;
	dir = opendir(path.c_str());
	if (!dir) {
		ESP_LOGE(tag, ">> Error opening folder\r\n");
		return;
	}

	// list the files and folders
	struct dirent *direntry;
	while ((direntry = readdir(dir)) != NULL) {

		// do not print the root folder (/spiffs)
		if(strcmp(direntry->d_name, "/spiffs") == 0) continue;

		if(direntry->d_type == DT_DIR) printf("DIR\t");
		else if(direntry->d_type == DT_REG) printf("FILE\t");
		else printf("???\t");
		ESP_LOGD(tag, ">> %s\r\n", direntry->d_name);
	}

	// close the folder
	closedir(dir);

}
