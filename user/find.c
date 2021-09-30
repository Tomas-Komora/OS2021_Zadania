#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void
find(char *dirName, char *file)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;
    //CHeck if  i can open descriptor
    if((fd = open(dirName, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", dirName);
        return;
    }
    //Inicialization status of  directory. 
    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", dirName);
        close(fd);
        return;
    }
    //Control if it is directory.
    if (st.type != T_DIR)
    {
        fprintf(2, "find: %s is not a directory\n", dirName);
        close(fd);
        return;
    }
    //Check if length of directory name is not too long
    if(strlen(dirName) + 1 + DIRSIZ + 1 > sizeof buf)
    {
        fprintf(2, "find: directory too long\n");
        close(fd);
        return;
    }
    //Make root of path. 
    strcpy(buf, dirName);
    p = buf+strlen(buf);
    *p++ = '/';
    //Step by step reading files in directory.
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if(de.inum == 0)
            continue;
        if (!strcmp(de.name, ".") || !strcmp(de.name, ".."))
            continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if(stat(buf, &st) < 0){
            fprintf(2,"find: cannot stat %s\n", buf);
            continue;
        }
        switch(st.type){

            case T_DIR:
                find(buf, file);
                break;
            case T_FILE:
                if(!strcmp(file, de.name))
                    printf("%s\n", buf);
                break; 
        }
    }
    close(fd);
}

int
main(int argc, char *argv[])
{
    //if argc is not 3 write errorand exit.
    if(argc != 3)
    {
        fprintf(2, "usage: find dir name file name\n");
        exit(-1);
    }
    find(argv[1], argv[2]);
    exit(0);
}
