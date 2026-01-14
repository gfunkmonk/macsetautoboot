#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#if defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__)
#include <sys/pciio.h>
#include <sys/ioctl.h>
#define BSD_PLATFORM 1
#elif defined(__linux__)
/* Linux platform - no additional defines needed */
#else
#error "Unsupported platform. This program only supports BSD and Linux systems."
#endif

/* Auto-boot enable value for Mac Mini */
#define AUTOBOOT_VALUE 0x19

int main(int argc, char **argv) {
	(void)argc;
	(void)argv;
	int pc_bus = 0;
	int pc_dev = 3;	/* device on this bus */
	int pc_func = 0;	/* function on this device */
	int reg = 0x78;	/* must be 32-bit aligned */
	uint32_t word;
	int ret;
	int fd;

#ifdef BSD_PLATFORM
	struct pci_io pcio;
	struct pcisel pcisel;

	fd = open("/dev/pci0", O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		if (errno == EACCES)
			perror("open /dev/pci0");
		else
			perror("open /dev/pci0: try securelevel 0 OR machdep.allowaperture>0");
		return -1;
	}

	// .. for an Intel Mac Mini
	// setpci -s 0:1f.0 0xa4.b=0
	// ... for an NVidia Mac Mini
	// setpci -s 00:03.0 0x7b.b=0x19
	// ... for a Unibody Mac Mini
	// setpci -s 0:3.0 -0x7b=20

	pcisel.pc_bus = pc_bus;
	pcisel.pc_dev = pc_dev;
	pcisel.pc_func = pc_func;

	/*
	 pi_sel - A pcisel structure - specifies the bus, slot and function
	 pi_reg - The PCI configuration register the user wants to access.
	 pi_width  The width, in bytes, of the data the user would like to
                       read.  This value can be only 4.
	 pi_data   The data returned by the kernel.
	*/
	pcio.pi_sel = pcisel;
	pcio.pi_reg = reg;
	//pcio.pi_reg = 0xa4;	/* must be 32-bit aligned */
	pcio.pi_width = 4;	/* "This value can only be 4" */

	ret = ioctl(fd, PCIOCREAD, &pcio);
	if (ret < 0) {
		perror("PCIOCREAD");
		close(fd);
		return -1;
	}
	word = htonl(pcio.pi_data);
#else
	/* Linux implementation using sysfs */
	char path[256];
	int path_len = snprintf(path, sizeof(path), "/sys/bus/pci/devices/0000:%02x:%02x.%x/config",
		pc_bus, pc_dev, pc_func);
	
	if (path_len < 0 || path_len >= (int)sizeof(path)) {
		fprintf(stderr, "Error: PCI device path too long\n");
		return -1;
	}
	
	fd = open(path, O_RDWR);
	if (fd < 0) {
		perror("open PCI config");
		fprintf(stderr, "Note: On Linux, you may need root privileges\n");
		return -1;
	}

	if (lseek(fd, reg, SEEK_SET) != reg) {
		perror("lseek");
		close(fd);
		return -1;
	}

	ret = read(fd, &word, 4);
	if (ret != 4) {
		perror("read");
		close(fd);
		return -1;
	}
	/* Linux config space is in native byte order */
	word = htonl(word);
#endif

	printf("%d:%d:%d offset 0x%x read ret=%d, data %08x", 
		pc_bus, pc_dev, pc_func,
		reg,
		ret, word);

	// If we get this far, change word and write it back.
	int nword = (word & 0xffffff00) | AUTOBOOT_VALUE;
	//int nword = word & 0x00ffffff;

	printf("-> %08x\n", nword);

#ifdef BSD_PLATFORM
	pcio.pi_data = ntohl(nword);
	ret = ioctl(fd, PCIOCWRITE, &pcio);
	if (ret < 0) {
		perror("PCIOCWRITE");
		close(fd);
		return -1;
	}
#else
	/* Linux implementation */
	if (lseek(fd, reg, SEEK_SET) != reg) {
		perror("lseek");
		close(fd);
		return -1;
	}

	uint32_t write_val = ntohl(nword);
	ret = write(fd, &write_val, 4);
	if (ret != 4) {
		perror("write");
		close(fd);
		return -1;
	}
#endif

	close(fd);
	return 0;
}
