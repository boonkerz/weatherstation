#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <errno.h>
#include "esp_err.h"
#include "esp_log.h"
#include <fcntl.h>
#include "esp_vfs.h"

using namespace std;

class FileSystem {
private:

public:
	void init();
	void ls(string path);
	int read(string path, uint8_t *buf, int &length);
};
