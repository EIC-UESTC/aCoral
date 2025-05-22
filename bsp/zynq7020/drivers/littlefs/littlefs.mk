DIR_SYS_LITTLEFS = ../../sys/littlefs
DIR_SYS_LITTLEFS_SRC = $(DIR_SYS_LITTLEFS)/src

STD_OBJS_LITTLEFS_H = -I $(DIR_SYS_LITTLEFS)/h 
STD_OBJS_LITTLEFS  = lfs.o lfs_util.o littlefs_io.o

lfs.o:$(DIR_SYS_LITTLEFS_SRC)/lfs.c
	$(XCC) $(FLAGS_C) $(DIR_INCS) $(DIR_SYS_LITTLEFS_SRC)/lfs.c 
lfs_util.o:$(DIR_SYS_LITTLEFS_SRC)/lfs_util.c
	$(XCC) $(FLAGS_C) $(DIR_INCS) $(DIR_SYS_LITTLEFS_SRC)/lfs_util.c
littlefs_io.o:$(DIR_SYS_LITTLEFS_SRC)/littlefs_io.c
	$(XCC) $(FLAGS_C) $(DIR_INCS) $(DIR_SYS_LITTLEFS_SRC)/littlefs_io.c