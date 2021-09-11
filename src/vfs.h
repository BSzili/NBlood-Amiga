#ifndef VFS_H
#define VFS_H

// EDuke32 compatibility

#define buildvfs_fd int
#define buildvfs_fd_invalid (-1)
#define buildvfs_kfd int32_t
#define buildvfs_kfd_invalid (-1)
#define buildvfs_FILE FILE *

#define buildvfs_fwrite(p, s, n, fp) fwrite((p), (s), (n), (fp))
#define buildvfs_fclose(fp) fclose(fp)
#define buildvfs_exists(fn) (access((fn), F_OK) == 0)
#define buildvfs_fopen_write(fn) fopen((fn), "wb")
#define buildvfs_chdir chdir
#define buildvfs_getcwd getcwd
#define buildvfs_write(fd, p, s) write((fd), (p), (s))
#define buildvfs_close(fd) close(fd)
#define buildvfs_open_write(fn) open((fn), O_BINARY|O_TRUNC|O_CREAT|O_WRONLY, S_IREAD|S_IWRITE)
#define buildvfs_open_read(fn) open((fn), O_RDONLY|O_BINARY)
#define buildvfs_read(fd, p, s) read((fd), (p), (s))
#define buildvfs_mkdir(dir, x) Bmkdir(dir, x)

static inline int buildvfs_isdir(char const *path)
{
	struct Bstat st;
	return (Bstat(path, &st) ? 0 : (st.st_mode & S_IFDIR) == S_IFDIR);
}

#ifdef __cplusplus
template <size_t N>
static inline void buildvfs_fputstr(buildvfs_FILE fp, char const (&str)[N])
{
	buildvfs_fwrite(&str, 1, N-1, fp);
}
#endif

static inline void buildvfs_fputstrptr(buildvfs_FILE fp, char const * str)
{
	buildvfs_fwrite(str, 1, strlen(str), fp);
}

#define BUILDVFS_FIND_REC CACHE1D_FIND_REC
#define BUILDVFS_FIND_FILE CACHE1D_FIND_FILE
#define BUILDVFS_FIND_DIR CACHE1D_FIND_DIR
#define BUILDVFS_SOURCE_GRP CACHE1D_SOURCE_GRP

#define klistaddentry(rec, name, type, source) (-1) // not supported
#define kfileparent(origfp) ((char *)NULL) // only for GRP files

#endif
