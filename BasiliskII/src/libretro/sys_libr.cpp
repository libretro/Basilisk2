/*
 *
 *  Basilisk II (C) Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "sysdeps.h"

#include "main.h"
#include "macos_util.h"
#include "prefs.h"
#include "user_strings.h"
#include "sys.h"
#include "disk_unix.h"

#if defined(BINCUE)
#include "bincue_unix.h"
#endif



#define DEBUG 0
#include "debug.h"

static disk_factory *disk_factories[] = {
#ifndef STANDALONE_GUI
	disk_sparsebundle_factory,
#if defined(HAVE_LIBVHD)
	disk_vhd_factory,
#endif
#endif
	NULL
};

// File handles are pointers to these structures
struct mac_file_handle {
	char *name;          // Copy of device/file name
	int fd;

	bool is_file;        // Flag: plain file or /dev/something?
	bool is_floppy;		// Flag: floppy device
	bool is_cdrom;       // Flag: CD-ROM device
	bool read_only;		// Copy of Sys_open() flag

	loff_t start_byte;	// Size of file header (if any)
	loff_t file_size;    // Size of file data (only valid if is_file is true)

	bool is_media_present;		// Flag: media is inserted and available
	disk_generic *generic_disk;

#if defined(BINCUE)
	bool is_bincue;		// Flag: BIN CUE file
	void *bincue_fd;
#endif
};

// Open file handles
struct open_mac_file_handle {
	mac_file_handle *fh;
	open_mac_file_handle *next;
};
static open_mac_file_handle *open_mac_file_handles = NULL;

// File handle of first floppy drive (for SysMountFirstFloppy())
static mac_file_handle *first_floppy = NULL;

// Prototypes
static void cdrom_close(mac_file_handle *fh);
static bool cdrom_open(mac_file_handle *fh, const char *path = NULL);


/*
 *  Initialization
 */

void SysInit(void) {}


/*
 *  Deinitialization
 */

void SysExit(void) {}


/*
 *  Manage open file handles
 */

static void sys_add_mac_file_handle(mac_file_handle *fh)
{
	open_mac_file_handle *p = new open_mac_file_handle;
	p->fh = fh;
	p->next = open_mac_file_handles;
	open_mac_file_handles = p;
}

static void sys_remove_mac_file_handle(mac_file_handle *fh)
{
	open_mac_file_handle *p = open_mac_file_handles;
	open_mac_file_handle *q = NULL;

	while (p) {
		if (p->fh == fh) {
			if (q)
				q->next = p->next;
			else
				open_mac_file_handles = p->next;
			delete p;
			break;
		}
		q = p;
		p = p->next;
	}
}


/*
 *  Account for media that has just arrived
 */

void SysMediaArrived(const char *path, int type)
{
   /*
	// Replace the "cdrom" entry (we are polling, it's unique)
	if (type == MEDIA_CD && !PrefsFindBool("nocdrom"))
		PrefsReplaceString("cdrom", path);

	// Wait for media to be available for reading
	if (open_mac_file_handles) {
		const int MAX_WAIT = 5;
		for (int i = 0; i < MAX_WAIT; i++) {
			if (access(path, R_OK) == 0)
				break;
			switch (errno) {
			case ENOENT: // Unlikely
			case EACCES: // MacOS X is mounting the media
				sleep(1);
				continue;
			}
			printf("WARNING: Cannot access %s (%s)\n", path, strerror(errno));
			return;
		}
	}

	for (open_mac_file_handle *p = open_mac_file_handles; p != NULL; p = p->next) {
		mac_file_handle * const fh = p->fh;

		// Re-open CD-ROM device
		if (fh->is_cdrom && type == MEDIA_CD) {
			cdrom_close(fh);
			if (cdrom_open(fh, path)) {
				fh->is_media_present = true;
				MountVolume(fh);
			}
		}
	}
    */
}


/*
 *  Account for media that has just been removed
 */

void SysMediaRemoved(const char *path, int type)
{
   /*
	if ((type & MEDIA_REMOVABLE) != MEDIA_CD)
		return;

	for (open_mac_file_handle *p = open_mac_file_handles; p != NULL; p = p->next) {
		mac_file_handle * const fh = p->fh;

		// Mark media as not available
		if (!fh->is_cdrom || !fh->is_media_present)
			continue;
		if (fh->name && strcmp(fh->name, path) == 0) {
			fh->is_media_present = false;
			break;
		}
#if defined __MACOSX__
		if (fh->ioctl_name && strcmp(fh->ioctl_name, path) == 0) {
			fh->is_media_present = false;
			break;
		}
#endif
	}
   */
}


/*
 *  Mount first floppy disk
 */

void SysMountFirstFloppy(void)
{
	if (first_floppy)
		MountVolume(first_floppy);
}


/*
 *  This gets called when no "floppy" prefs items are found
 *  It scans for available floppy drives and adds appropriate prefs items
 */

void SysAddFloppyPrefs(void) {}


/*
 *  This gets called when no "disk" prefs items are found
 *  It scans for available HFS volumes and adds appropriate prefs items
 *	On OS X, we could do the same, but on an OS X machine I think it is
 *	very unlikely that any mounted volumes would contain a system which
 *	is old enough to boot a 68k Mac, so we just do nothing here for now.
 */

void SysAddDiskPrefs(void) {}


/*
 *  This gets called when no "cdrom" prefs items are found
 *  It scans for available CD-ROM drives and adds appropriate prefs items
 */

void SysAddCDROMPrefs(void) {}


/*
 *  Add default serial prefs (must be added, even if no ports present)
 */

void SysAddSerialPrefs(void) {}


/*
 *  Open CD-ROM device and initialize internal data
 */

static bool cdrom_open_1(mac_file_handle *fh)
{
	return true;
}

bool cdrom_open(mac_file_handle *fh, const char *path)
{
	if (path)
		fh->name = strdup(path);
	fh->fd = open(fh->name, O_RDONLY, O_NONBLOCK);
	fh->start_byte = 0;
	if (!cdrom_open_1(fh))
		return false;
	return fh->fd >= 0;
}


/*
 *  Close a CD-ROM device
 */

void cdrom_close(mac_file_handle *fh)
{

	if (fh->fd >= 0) {
		close(fh->fd);
		fh->fd = -1;
	}
	if (fh->name) {
		free(fh->name);
		fh->name = NULL;
	}
}


/*
 *  Check if device is a mounted HFS volume, get mount name
 */

static bool is_drive_mounted(const char *dev_name, char *mount_name)
{
	return false;
}


/*
 *  Open file/device, create new file handle (returns NULL on error)
 */
 
static mac_file_handle *open_filehandle(const char *name)
{
		mac_file_handle *fh = new mac_file_handle;
		memset(fh, 0, sizeof(mac_file_handle));
		fh->name = strdup(name);
		fh->fd = -1;
		fh->generic_disk = NULL;
		return fh;
}

void *Sys_open(const char *name, bool read_only)
{
	bool is_file = true;
	bool is_cdrom = false;
	bool is_floppy = false;

	bool is_polled_media = false;

	D(bug("Sys_open(%s, %s)\n", name, read_only ? "read-only" : "read/write"));

	// Check if write access is allowed, set read-only flag if not
	if (!read_only && access(name, W_OK))
		read_only = true;

	// Print warning message and eventually unmount drive when this is an HFS volume mounted under Linux (double mounting will corrupt the volume)
	char mount_name[256];
	if (!is_file && !read_only && is_drive_mounted(name, mount_name)) {
		char str[512];
		sprintf(str, GetString(STR_VOLUME_IS_MOUNTED_WARN), mount_name);
		WarningAlert(str);
		sprintf(str, "umount %s", mount_name);
		if (system(str)) {
			sprintf(str, GetString(STR_CANNOT_UNMOUNT_WARN), mount_name, strerror(errno));
			WarningAlert(str);
			return NULL;
		}
	}

	// Open file/device

#if defined(BINCUE)
	void *binfd = open_bincue(name);
	if (binfd) {
		mac_file_handle *fh = open_filehandle(name);
		D(bug("opening %s as bincue\n", name));
		fh->bincue_fd = binfd;
		fh->is_bincue = true;
		fh->read_only = true;
		fh->is_media_present = true;
		sys_add_mac_file_handle(fh);
		return fh;
	}
#endif


	for (int i = 0; disk_factories[i]; ++i) {
		disk_factory *f = disk_factories[i];
		disk_generic *generic;
		disk_generic::status st = f(name, read_only, &generic);
		if (st == disk_generic::DISK_INVALID)
			return NULL;
		if (st == disk_generic::DISK_VALID) {
			mac_file_handle *fh = open_filehandle(name);
			fh->generic_disk = generic;
			fh->file_size = generic->size();
			fh->read_only = generic->is_read_only();
			fh->is_media_present = true;
			sys_add_mac_file_handle(fh);
			return fh;
		}
	}

	int open_flags = (read_only ? O_RDONLY : O_RDWR);
#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__MACOSX__)
	open_flags |= (is_cdrom ? O_NONBLOCK : 0);
#endif
#if defined(__MACOSX__)
	open_flags |= (is_file ? O_EXLOCK | O_NONBLOCK : 0);
#endif
	int fd = open(name, open_flags);
#if defined(__MACOSX__)
	if (fd < 0 && (open_flags & O_EXLOCK)) {
		if (errno == EOPNOTSUPP) {
			// File system does not support locking. Try again without.
			open_flags &= ~O_EXLOCK;
			fd = open(name, open_flags);
		} else if (errno == EAGAIN) {
			// File is likely already locked by another process.
			printf("WARNING: Cannot open %s (%s)\n", name, strerror(errno));
			return NULL;
		}
	}
#endif
	if (fd < 0 && !read_only) {
		// Read-write failed, try read-only
		read_only = true;
		fd = open(name, O_RDONLY);
	}
	if (fd >= 0 || is_polled_media) {
		mac_file_handle *fh = open_filehandle(name);
		fh->fd = fd;
		fh->is_file = is_file;
		fh->read_only = read_only;
		fh->is_floppy = is_floppy;
		fh->is_cdrom = is_cdrom;
		if (fh->is_file) {
			fh->is_media_present = true;
			// Detect disk image file layout
			loff_t size = 0;
			size = lseek(fd, 0, SEEK_END);
			uint8 data[256];
			lseek(fd, 0, SEEK_SET);
			read(fd, data, 256);
			FileDiskLayout(size, data, fh->start_byte, fh->file_size);
		} else {
			struct stat st;
			if (fstat(fd, &st) == 0) {
				fh->is_media_present = true;
				if (S_ISBLK(st.st_mode)) {
					fh->is_cdrom = is_cdrom;
#if defined(__linux__)
					fh->is_floppy = (MAJOR(st.st_rdev) == FLOPPY_MAJOR);
#ifdef CDROM_GET_CAPABILITY
					if (is_cdrom) {
						fh->cdrom_cap = ioctl(fh->fd, CDROM_GET_CAPABILITY);
						if (fh->cdrom_cap < 0)
							fh->cdrom_cap = 0;
					}
#endif
#elif defined(__FreeBSD__)
					fh->is_floppy = ((st.st_rdev >> 16) == 2);
#ifdef CDIOCCAPABILITY
					if (is_cdrom) {
						if (ioctl(fh->fd, CDIOCCAPABILITY, &fh->cdrom_cap) < 0)
							memset(&fh->cdrom_cap, 0, sizeof(fh->cdrom_cap));
					}
#endif
#elif defined(__NetBSD__)
					fh->is_floppy = ((st.st_rdev >> 16) == 2);
#endif
				}
#if defined __MACOSX__
				if (is_cdrom) {
					fh->is_cdrom = true;
					fh->is_floppy = false;
					if (cdrom_open_1(fh))
						fh->is_media_present = true;
				}
#endif
			}
		}
		if (fh->is_floppy && first_floppy == NULL)
			first_floppy = fh;
		sys_add_mac_file_handle(fh);
		return fh;
	} else {
		printf("WARNING: Cannot open %s (%s)\n", name, strerror(errno));
		return NULL;
	}
}


/*
 *  Close file/device, delete file handle
 */

void Sys_close(void *arg)
{
	mac_file_handle *fh = (mac_file_handle *)arg;
	if (!fh)
		return;

	sys_remove_mac_file_handle(fh);

#if defined(BINCUE)
	if (fh->is_bincue)
		close_bincue(fh->bincue_fd);
#endif
	if (fh->generic_disk)
		delete fh->generic_disk;

	if (fh->is_cdrom)
		cdrom_close(fh);
	if (fh->fd >= 0)
		close(fh->fd);
	if (fh->name)
		free(fh->name);
	delete fh;
}


/*
 *  Read "length" bytes from file/device, starting at "offset", to "buffer",
 *  returns number of bytes read (or 0)
 */

size_t Sys_read(void *arg, void *buffer, loff_t offset, size_t length)
{
	mac_file_handle *fh = (mac_file_handle *)arg;
	if (!fh)
		return 0;

#if defined(BINCUE)
	if (fh->is_bincue)
		return read_bincue(fh->bincue_fd, buffer, offset, length);
#endif

	if (fh->generic_disk)
		return fh->generic_disk->read(buffer, offset, length);
	
	// Seek to position
	if (lseek(fh->fd, offset + fh->start_byte, SEEK_SET) < 0)
		return 0;

	// Read data
	return read(fh->fd, buffer, length);
}


/*
 *  Write "length" bytes from "buffer" to file/device, starting at "offset",
 *  returns number of bytes written (or 0)
 */

size_t Sys_write(void *arg, void *buffer, loff_t offset, size_t length)
{
	mac_file_handle *fh = (mac_file_handle *)arg;
	if (!fh)
		return 0;

	if (fh->generic_disk)
		return fh->generic_disk->write(buffer, offset, length);

	// Seek to position
	if (lseek(fh->fd, offset + fh->start_byte, SEEK_SET) < 0)
		return 0;

	// Write data
	return write(fh->fd, buffer, length);
}


/*
 *  Return size of file/device (minus header)
 */

loff_t SysGetFileSize(void *arg)
{
	mac_file_handle *fh = (mac_file_handle *)arg;
	if (!fh)
		return 0;//was return true

#if defined(BINCUE)
	if (fh->is_bincue)
		return size_bincue(fh->bincue_fd);
#endif 

	if (fh->generic_disk)
		return fh->file_size;

	if (fh->is_file)
		return fh->file_size;
   
   return 0;
}


/*
 *  Eject volume (if applicable)
 */

void SysEject(void *arg) {}


/*
 *  Format volume (if applicable)
 */

bool SysFormat(void *arg)
{
	mac_file_handle *fh = (mac_file_handle *)arg;
	if (!fh)
		return false;

	//!!
	return true;
}


/*
 *  Check if file/device is read-only (this includes the read-only flag on Sys_open())
 */

bool SysIsReadOnly(void *arg)
{
	mac_file_handle *fh = (mac_file_handle *)arg;
	if (!fh)
		return true;
   
   return fh->read_only;
}


/*
 *  Check if the given file handle refers to a fixed or a removable disk
 */

bool SysIsFixedDisk(void *arg)
{
	mac_file_handle *fh = (mac_file_handle *)arg;
	if (!fh)
		return true;

	if (fh->generic_disk)
		return true;

	if (fh->is_file)
		return true;

   return true;
}


/*
 *  Check if a disk is inserted in the drive (always true for files)
 */

bool SysIsDiskInserted(void *arg)
{
	mac_file_handle *fh = (mac_file_handle *)arg;
	if (!fh)
		return false;

	if (fh->generic_disk)
		return true;
	
	if (fh->is_file)
		return true;
   
   //was originally return true by default.
   return false;
}


/*
 *  Prevent medium removal (if applicable)
 */

void SysPreventRemoval(void *arg) {}


/*
 *  Allow medium removal (if applicable)
 */

void SysAllowRemoval(void *arg) {}


/*
 *  Read CD-ROM TOC (binary MSF format, 804 bytes max.)
 */

bool SysCDReadTOC(void *arg, uint8 *toc)
{
	mac_file_handle *fh = (mac_file_handle *)arg;
	if (!fh)
		return false;

#if defined(BINCUE)
	if (fh->is_bincue)
		return readtoc_bincue(fh->bincue_fd, toc);
#endif

   return false;
}


/*
 *  Read CD-ROM position data (Sub-Q Channel, 16 bytes, see SCSI standard)
 */

bool SysCDGetPosition(void *arg, uint8 *pos)
{
	mac_file_handle *fh = (mac_file_handle *)arg;
	if (!fh)
		return false;

#if defined(BINCUE)
	if (fh->is_bincue)
		return GetPosition_bincue(fh->bincue_fd, pos);
#endif
   
   return false;
}


/*
 *  Play CD audio
 */

bool SysCDPlay(void *arg, uint8 start_m, uint8 start_s, uint8 start_f, uint8 end_m, uint8 end_s, uint8 end_f)
{
	mac_file_handle *fh = (mac_file_handle *)arg;
	if (!fh)
		return false;

#if defined(BINCUE)
	if (fh->is_bincue)
		return CDPlay_bincue(fh->bincue_fd, start_m, start_s, start_f, end_m, end_s, end_f);
#endif
   
   return false;
}


/*
 *  Pause CD audio
 */

bool SysCDPause(void *arg)
{
	mac_file_handle *fh = (mac_file_handle *)arg;
	if (!fh)
		return false;

#if defined(BINCUE)
	if (fh->is_bincue)
		return CDPause_bincue(fh->bincue_fd);
#endif

   return false;
}


/*
 *  Resume paused CD audio
 */

bool SysCDResume(void *arg)
{
	mac_file_handle *fh = (mac_file_handle *)arg;
	if (!fh)
		return false;

#if defined(BINCUE)
	if (fh->is_bincue)
		return CDResume_bincue(fh->bincue_fd);
#endif

   return false;
}


/*
 *  Stop CD audio
 */

bool SysCDStop(void *arg, uint8 lead_out_m, uint8 lead_out_s, uint8 lead_out_f)
{
	mac_file_handle *fh = (mac_file_handle *)arg;
	if (!fh)
		return false;

#if defined(BINCUE)
	if (fh->is_bincue)
		return CDStop_bincue(fh->bincue_fd);
#endif

   return false;
}


/*
 *  Perform CD audio fast-forward/fast-reverse operation starting from specified address
 */

bool SysCDScan(void *arg, uint8 start_m, uint8 start_s, uint8 start_f, bool reverse)
{
   return false;
   /*
	mac_file_handle *fh = (mac_file_handle *)arg;
	if (!fh)
		return false;

	// Not supported under Linux
	return false;
   */
}


/*
 *  Set CD audio volume (0..255 each channel)
 */

void SysCDSetVolume(void *arg, uint8 left, uint8 right) {}


/*
 *  Get CD audio volume (0..255 each channel)
 */

void SysCDGetVolume(void *arg, uint8 &left, uint8 &right)
{
	mac_file_handle *fh = (mac_file_handle *)arg;
	if (!fh)
		return;

	left = right = 0;
}
