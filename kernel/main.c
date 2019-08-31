
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Forrest Yu, 2005
Chenghang Shi, 2019/08
Liang Wang, 2019/08
Hua Jiang, 2019/08
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"

#include "ano_schdule.h"


/*****************************************************************************
*                               kernel_main
*****************************************************************************/
/**
* jmp from kernel.asm::_start.
*
*****************************************************************************/
char current_dirr[512] = "/";

char currentUser[128] = "/";
char currentFolder[128] = "|";
char filepath[128] = "";
char users[2][128] = {"empty", "empty"};
char passwords[2][128];
char files[20][128];
char userfiles[20][128];
int filequeue[50];
int filecount = 0;
int usercount = 0;
int isEntered = 0;
int UserState = 0;
//int UserSwitch = 0;
int leiflag = 0;

PUBLIC int kernel_main()
{
    disp_str("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

    int i, j, eflags, prio;
    u8 rpl;
    u8 priv; /* privilege */

    struct task *t;
    struct proc *p = proc_table;
    char *stk = task_stack + STACK_SIZE_TOTAL;

    for (i = 0; i < NR_TASKS + NR_PROCS; i++, p++, t++)
    {
        if (i >= NR_TASKS + NR_NATIVE_PROCS)
        {
            p->p_flags = FREE_SLOT;
            continue;
        }

        if (i < NR_TASKS)
        { /* TASK */
            t = task_table + i;
            priv = PRIVILEGE_TASK;
            rpl = RPL_TASK;
            eflags = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
            prio = 15;
        }
        else
        { /* USER PROC */
            t = user_proc_table + (i - NR_TASKS);
            priv = PRIVILEGE_USER;
            rpl = RPL_USER;
            eflags = 0x202; /* IF=1, bit 2 is always 1 */
            prio = 5;
        }

        strcpy(p->name, t->name); /* name of the process */
        p->p_parent = NO_TASK;

        if (strcmp(t->name, "INIT") != 0)
        {
            p->ldts[INDEX_LDT_C] = gdt[SELECTOR_KERNEL_CS >> 3];
            p->ldts[INDEX_LDT_RW] = gdt[SELECTOR_KERNEL_DS >> 3];

            /* change the DPLs */
            p->ldts[INDEX_LDT_C].attr1 = DA_C | priv << 5;
            p->ldts[INDEX_LDT_RW].attr1 = DA_DRW | priv << 5;
        }
        else
        { /* INIT process */
            unsigned int k_base;
            unsigned int k_limit;
            int ret = get_kernel_map(&k_base, &k_limit);
            assert(ret == 0);
            init_desc(&p->ldts[INDEX_LDT_C],
                      0, /* bytes before the entry point
				   * are useless (wasted) for the
				   * INIT process, doesn't matter
				   */
                      (k_base + k_limit) >> LIMIT_4K_SHIFT,
                      DA_32 | DA_LIMIT_4K | DA_C | priv << 5);

            init_desc(&p->ldts[INDEX_LDT_RW],
                      0, /* bytes before the entry point
				   * are useless (wasted) for the
				   * INIT process, doesn't matter
				   */
                      (k_base + k_limit) >> LIMIT_4K_SHIFT,
                      DA_32 | DA_LIMIT_4K | DA_DRW | priv << 5);
        }

        p->regs.cs = INDEX_LDT_C << 3 | SA_TIL | rpl;
        p->regs.ds =
            p->regs.es =
                p->regs.fs =
                    p->regs.ss = INDEX_LDT_RW << 3 | SA_TIL | rpl;
        p->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;
        p->regs.eip = (u32)t->initial_eip;
        p->regs.esp = (u32)stk;
        p->regs.eflags = eflags;

        p->ticks = p->priority = prio;
        set_proc_rank(p,0);

        strcpy(p->name, t->name); /* name of the process */
        p->pid = i;               /* pid */
        p->run_count = 0;
        p->run_state = 1;

        p->p_flags = 0;
        p->p_msg = 0;
        p->p_recvfrom = NO_TASK;
        p->p_sendto = NO_TASK;
        p->has_int_msg = 0;
        p->q_sending = 0;
        p->next_sending = 0;

        for (j = 0; j < NR_FILES; j++)
            p->filp[j] = 0;

        stk -= t->stacksize;
    }

    k_reenter = 0;
    ticks = 0;

    p_proc_ready = proc_table;


    init_clock();
    init_keyboard();

    restart();


    while (1)
    {
    }
}

/*****************************************************************************
*                                get_ticks
*****************************************************************************/
PUBLIC int get_ticks()
{
    MESSAGE msg;
    reset_msg(&msg);
    msg.type = GET_TICKS;
    send_recv(BOTH, TASK_SYS, &msg);
    return msg.RETVAL;
}

/**
* @struct posix_tar_header
* Borrowed from GNU `tar'
*/
struct posix_tar_header
{                       /* byte offset */
    char name[100];     /*   0 */
    char mode[8];       /* 100 */
    char uid[8];        /* 108 */
    char gid[8];        /* 116 */
    char size[12];      /* 124 */
    char mtime[12];     /* 136 */
    char chksum[8];     /* 148 */
    char typeflag;      /* 156 */
    char linkname[100]; /* 157 */
    char magic[6];      /* 257 */
    char version[2];    /* 263 */
    char uname[32];     /* 265 */
    char gname[32];     /* 297 */
    char devmajor[8];   /* 329 */
    char devminor[8];   /* 337 */
    char prefix[155];   /* 345 */
                        /* 500 */
};

/*****************************************************************************
*                                untar
*****************************************************************************/
/**
* Extract the tar file and store them.
*
* @param filename The tar file.
*****************************************************************************/
void untar(const char *filename)
{
    printf("[extract `%s'\n", filename);
    int fd = open(filename, O_RDWR);
    assert(fd != -1);

    char buf[SECTOR_SIZE * 16];
    int chunk = sizeof(buf);
    int i = 0;
    int bytes = 0;

    while (1)
    {
        bytes = read(fd, buf, SECTOR_SIZE);
        assert(bytes == SECTOR_SIZE); /* size of a TAR file
									  * must be multiple of 512
									  */
        if (buf[0] == 0)
        {
            if (i == 0)
                printf("    need not unpack the file.\n");
            break;
        }
        i++;

        struct posix_tar_header *phdr = (struct posix_tar_header *)buf;

        /* calculate the file size */
        char *p = phdr->size;
        int f_len = 0;
        while (*p)
            f_len = (f_len * 8) + (*p++ - '0'); /* octal */

        int bytes_left = f_len;
        int fdout = open(phdr->name, O_CREAT | O_RDWR | O_TRUNC);
        if (fdout == -1)
        {
            printf("    failed to extract file: %s\n", phdr->name);
            printf(" aborted]\n");
            close(fd);
            return;
        }
        printf("    %s\n", phdr->name);
        while (bytes_left)
        {
            int iobytes = min(chunk, bytes_left);
            read(fd, buf,
                 ((iobytes - 1) / SECTOR_SIZE + 1) * SECTOR_SIZE);
            bytes = write(fdout, buf, iobytes);
            assert(bytes == iobytes);
            bytes_left -= iobytes;
        }
        close(fdout);
    }

    if (i)
    {
        lseek(fd, 0, SEEK_SET);
        buf[0] = 0;
        bytes = write(fd, buf, 1);
        assert(bytes == 1);
    }

    close(fd);

    printf(" done, %d files extracted]\n", i);
}

/*****************************************************************************
*                                shabby_shell
*****************************************************************************/
/**
* A very very simple shell.
*
* @param tty_name  TTY file name.
*****************************************************************************/
PUBLIC void clear()
{
    int i = 0;
    for (i = 0; i < 30; i++)
        printf("\n");
}

void shabby_shell(const char *tty_name)
{
    int fd_stdin = open(tty_name, O_RDWR);
    assert(fd_stdin == 0);
    int fd_stdout = open(tty_name, O_RDWR);
    assert(fd_stdout == 1);

    char rdbuf[256]; //读取的命令
    char cmd[128];   //指令
    char arg1[128];  //参数1
    char arg2[128];  //参数2
    char buf[1024];

    initFs();
    while (1)
    {
        if (usercount == 0)
        {
            printf("Enter Admin Password:");
            char buf[128];
            int r = read(0, buf, 128);
            buf[r] = 0;
            if (strcmp(buf, "admin") == 0)
            {
                strcpy(currentUser, "/");
                UserState = 3;
                break;
            }
            else
                printf("Password Error!\n");
        }
        else
        {
            //printf("%d",usercount);
            int isGet = 0;
            printf("Enter User Name:");
            char buf[128];
            int r = read(0, buf, 128);
            buf[r] = 0;
            int i;
            for (i = 0; i < usercount; i++)
            {
                if (strcmp(buf, users[i]) == 0 && strcmp(buf, "empty") != 0)
                {
                    printf("Enter %s's Password:", users[i]);
                    char buf[128];
                    int r = read(0, buf, 128);
                    buf[r] = 0;
                    if (strcmp(buf, passwords[i]) == 0)
                    {
                        strcpy(currentUser, users[i]);
                        UserState = i + 1;
                        isGet = 1;
                        break;
                    }
                }
            }
            if (isGet)
                break;
            else
                printf("Password Error Or User Not Exist!\n");
        }
    }

    while (1)
    {
        //init char array
        clearArr(rdbuf, 128);
        clearArr(cmd, 128);
        clearArr(arg1, 128);
        clearArr(arg2, 128);
        clearArr(buf, 1024);
	if(UserState == 3)
		printf("[Admin@SOS]%s%s# ",currentUser,currentFolder);
	else	
		printf("[%s@SOS]%s$ ",currentUser, current_dirr);
        //write(1, "$ ", 2);
        int r = read(0, rdbuf, 70);
        rdbuf[r] = 0;

        int argc = 0;
        char *argv[PROC_ORIGIN_STACK];
        char *p = rdbuf;
        char *s;
        int word = 0;
        char ch;
        do
        {
            ch = *p;
            if (*p != ' ' && *p != 0 && !word)
            {
                s = p;
                word = 1;
            }
            if ((*p == ' ' || *p == 0) && word)
            {
                word = 0;
                argv[argc++] = s;
                *p = 0;
            }
            p++;
        } while (ch);
        argv[argc] = 0;

        int fd = open(argv[0], O_RDWR);

        if (fd == -1)
        {
            if (rdbuf[0])
            {
                int i = 0, j = 0;
                /* get command */
                while (rdbuf[i] != ' ' && rdbuf[i] != 0)
                {
                    cmd[i] = rdbuf[i];
                    i++;
                }
                i++;
                /* get arg1 */
                while (rdbuf[i] != ' ' && rdbuf[i] != 0)
                {
                    arg1[j] = rdbuf[i];
                    i++;
                    j++;
                }
                i++;
                j = 0;
                /* get arg2 */
                while (rdbuf[i] != ' ' && rdbuf[i] != 0)
                {
                    arg2[j] = rdbuf[i];
                    i++;
                    j++;
                }

                //Process the command to format"cmd arg1 arg2"
                if (strcmp(cmd, "help") == 0)
                {
                    showhelp();
                }
                else if (strcmp(cmd, "clear") == 0)
                {
                    clear();
                    welcome();
                } 
                else if (strcmp(cmd, "sudo") == 0)
                {
                    printf("Enter Admin Password:");
                    char buf[128];
                    int r = read(0, buf, 128);
                    buf[r] = 0;
                    if (strcmp(buf, "admin") == 0)
                    {
                        strcpy(currentUser, "/");
                        UserState = 3;
                    }
                    else
                        printf("Password Error!\n");
                }
                else if (strcmp(cmd, "adduser") == 0)
                {
                    addUser(arg1, arg2);
                }
                else if (strcmp(cmd, "deluser") == 0)
                {
                    moveUser(arg1, arg2);
                }
                else if (strcmp(cmd, "su") == 0)
                {
                    shift(arg1, arg2);
                }
                else if (strcmp(cmd, "mkfile") == 0||strcmp(cmd, "touch")==0)
                {
                    CreateFile(current_dirr, arg1);
                }
		else if(strcmp(cmd, "mkdir") == 0)
		{
            CreateDir(current_dirr, arg1);
		}
		else if (strcmp(cmd, "cd") == 0) 
		{
            GoDir(current_dirr, arg1);
		}
                else if (strcmp(cmd, "rd") == 0)
                {
                    ReadFile(current_dirr, arg1);
                }
                else if (strcmp(cmd, "vi") == 0)
                {
                    textPad(current_dirr, arg1);
                }
                /* edit a file appand */
                else if (strcmp(cmd, "wt+") == 0)
                {
                    new_editAppand(current_dirr, arg1, arg2);
                }
                /* edit a file cover */
                else if (strcmp(cmd, "wt") == 0)
                {
                    new_editCover(current_dirr, arg1, arg2);
                }
                /* delete a file */
                else if (strcmp(cmd, "del") == 0)
                {
                    DeleteFile(current_dirr, arg1);
                }
                /* ls */
                else if (strcmp(cmd, "ls") == 0)
                {
                    ls(current_dirr);
                }
                else if (strcmp(cmd, "proc") == 0)
                {
                    showProcess();
                }
                else if (strcmp(cmd, "kill") == 0)
                {
                    killpro(arg1);
                }
                else if (strcmp(cmd, "pause") == 0)
                {
                    pausepro(arg1);
                }
                else if (strcmp(cmd, "resume") == 0)
                {
                    resume(arg1);
                }
                else if(strcmp(cmd, "date") == 0)
                {
                    my_date();
                }
                else if (strcmp(cmd, "tictactoe") == 0||strcmp(cmd, "ttt")==0)
                {
                    main_tic();

                }
		else if(strcmp(cmd, "boom") == 0)
		{
		    mainboom();
		}
        else if(strcmp(cmd, "policy")==0)
        {
            printf("current policy: ");
            printf(get_schd_policy()==POLICY_PRI?"priority":"multi-que");
            printf("!\n");
        }
        else if(strcmp(cmd, "changeplc")==0)
        {
            change_schd_policy();
        }
        //test handmade scanf
        else if(strcmp(cmd, "tscanf")==0){
            printf("input format like : %%s%%d%%c%%c%%d%%s\n");
            char input1[128], input2[128];
            int d1, d2;
            char c1, c2;
            scanf("%s%d%c%c%d%s",input1, &d1, &c1, &c2, &d2, input2);

            printf("%s\n%d\n%c\n%c\n%d\n%s\n",input1, d1, c1, c2, d2, input2);
        }
        else
        {
            printf("i cannot understand your cmd\n");
            continue;
        }
            }
        }
        else
        {
            close(fd);
            int pid = fork();
            if (pid != 0)
            { /* parent */
                int s;
                wait(&s);
            }
            else
            { /* child */
                execv(argv[0], argv);
            }
        }
    }

    close(1);
    close(0);
}

void killpro(char *a)
{
    int b = *a - 48;
    if(b >= 0 && b <= NR_TASKS)   printf("operation to proc %d denied \n",b);
    else if(b < NR_TASKS + NR_NATIVE_PROCS)
    {
        proc_table[b].p_flags = 1;
        showProcess();
    }
    else printf("can not find proc with pid:%d \n",b);
}

void pausepro(char *a)
{
    int b = *a - 48;
    if(b >= 0 && b <= NR_TASKS)   printf("operation to proc %d denied \n",b);
    else if(b < NR_TASKS + NR_NATIVE_PROCS)
    {
        proc_table[b].run_state = 0;
        showProcess();
    }
    else printf("can not find proc with pid:%d \n",b);
}

void resume(char *a)
{
    int b = *a - 48;
    if(b >= 0 && b <= NR_TASKS)   printf("operation to proc %d denied \n",b);
    else if(b < NR_TASKS + NR_NATIVE_PROCS)
    {
        proc_table[b].run_state = 1;
        showProcess();
    }
    else printf("can not find proc with pid:%d \n",b);
}

void clearArr(char *arr, int length)
{
    int i;
    for (i = 0; i < length; i++)
        arr[i] = 0;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Multilevel File System
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Wang Liang, 2019
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

int mkdir(char* path)
{
    MESSAGE msg;
    msg.type = MKDIR;

    msg.PATHNAME = (void*)path;
    msg.NAME_LEN = strlen(path);
    msg.FLAGS = 0;

    send_recv(BOTH, TASK_FS, &msg);

    return msg.RETVAL;
}

/*****************************************************************************
 *                                convert_to_absolute
 *                      将传入的路径和文件名组合成一个完整的绝对路径
 *****************************************************************************/
PUBLIC void convert_to_absolute(char* dest, char* path, char* file)
{
    int i=0, j=0;
    while (path[i] != 0)  // 写入路径
    {
        dest[j] = path[i];
        j++;
        i++;
    }
    i = 0;
    while (file[i] != 0)  // 写入文件名
    {
        dest[j] = file[i];
        j++;
        i++;
    }
    dest[j] = 0;  // 结束符
}

void CreateDir(char* path, char* file)
{
    char absoPath[512];
    convert_to_absolute(absoPath, path, file);
    int fd = open(absoPath, O_RDWR);

    if (fd != -1)
    {
        printf("Failed to create a new directory with name %s\n", file);  // 文件夹不能与已有文件重名
        return;
    }
    // printf("absoPath is %s\n", absoPath);
    mkdir(absoPath);
}

/* Get File Pos */
int getPos()
{
    int i = 0;
    for (i = 0; i < 500; i++)
    {
        if (filequeue[i] == 1)
            return i;
    }
}

int len(char *a)
{
    int ans = 0;
    int i;
    for (i = 0; i < 16; i++)
    {
        if (a[i] == 0)
            break;
        ans++;
    }
    return ans;
}

int vertify()
{
    if (UserState == 0)
    {
        printf("Permission deny!!\n");
        return 0;
    }
    else
        return 1;
}

/* Create Filepath */
void createFilepath(char* filename)
{
	int i = 0, j = 0, k = 0;
		
	for (i; i < len(currentUser); i++)
	{
		filepath[i] = currentUser[i];
	}

	for (j; j < strlen(currentFolder); i++, j++) {
		filepath[i] = currentFolder[j];
	}

	for(k = 0; k < strlen(filename); i++, k++)
	{	
		filepath[i] = filename[k];
	}
	filepath[i] = '\0';
}

/* Update FileLogs */
void updateFileLogs()
{
    int i = 0, count = 0;
    editCover("fileLogs", "");
    while (count <= filecount - 1)
    {
        if (filequeue[i] == 0)
        {
            i++;
            continue;
        }
        char filename[128];
        int len = strlen(files[count]);
        strcpy(filename, files[count]);
        filename[len] = ' ';
        filename[len + 1] = '\0';
        //printf("%s\n", filename);
        editAppand("fileLogs", filename);
        count++;
        i++;
    }
}

/* Update myUsers */
void updateMyUsers()
{
    int i = 0, count = 0;
    editCover("myUsers", "");
    if (strcmp(users[0], "empty") != 0)
    {
        editAppand("myUsers", users[0]);
        editAppand("myUsers", " ");
    }
    else
    {
        editAppand("myUsers", "empty ");
    }
    if (strcmp(users[1], "empty") != 0)
    {
        editAppand("myUsers", users[1]);
        editAppand("myUsers", " ");
    }
    else
    {
        editAppand("myUsers", "empty ");
    }
}

/* Update myUsersPassword */
void updateMyUsersPassword()
{
    int i = 0, count = 0;
    editCover("myUsersPassword", "");
    if (strcmp(passwords[0], "") != 0)
    {
        editAppand("myUsersPassword", passwords[0]);
        editAppand("myUsersPassword", " ");
    }
    else
    {
        editAppand("myUsersPassword", "empty ");
    }
    if (strcmp(passwords[1], "") != 0)
    {
        editAppand("myUsersPassword", passwords[1]);
        editAppand("myUsersPassword", " ");
    }
    else
    {
        editAppand("myUsersPassword", "empty ");
    }
}

/* Add FIle Log */
void addLog(char * filepath)
{
	int pos = -1, i = 0;
	pos = getPos();
	filecount++;
	strcpy(files[pos], filepath);
	updateFileLogs();
	filequeue[pos] = 0;
	if (strcmp("/", currentUser) != 0)
	{
		int fd = -1, k = 0, j = 0;
		char filename[128];
		while (k < strlen(filepath))
		{
			if (filepath[k] != '|')
				k++;
			else
				break;
		}
		k++;
		while (k < strlen(filepath))
		{
			filename[j] = filepath[k];
			k++;
			j++;
		}
		filename[j] = '\0';
		if (strcmp(currentUser, users[0]) == 0)
		{
			editAppand("user1", filename);
			editAppand("user1", " ");
		}
		else if(strcmp(currentUser, users[1]) == 0)
		{
			editAppand("user2", filename);
			editAppand("user2", " ");
		}
	}
}

/* Delete File Log */
void deleteLog(char *filepath)
{
    int i = 0, fd = -1;
    for (i = 0; i < filecount; i++)
    {
        if (strcmp(filepath, files[i]) == 0)
        {
            strcpy(files[i], "empty");
            int len = strlen(files[i]);
            files[i][len] = '0' + i;
            files[i][len + 1] = '\0';
            fd = open(files[i], O_CREAT | O_RDWR);
            close(fd);
            filequeue[i] = 1;
            break;
        }
    }
    filecount--;
    updateFileLogs();
}

void showhelp()
{
    printf(" _________________________________________________________________ \n");
    printf("|          instruction           |             function           |\n");
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("| help                           | show help table                |\n");
    printf("| sudo                           | obtain administrator privileges|\n");
    printf("| adduser  [username] [password] | add user                       |\n");
    printf("| deluser  [username] [password] | remove user                    |\n");
    printf("| su       [username] [password] | shift to user                  |\n");
    printf("| ls                             | show file list                 |\n");
    printf("| mkdir    [foldername]          | create folder                  |\n");
    printf("| cd       [foldername]          | open folder                    |\n");
    printf("| rd       [filename]            | read file                      |\n");
    printf("| mkfile   [filename] [content]  | create file                    |\n");
    printf("| wt  +    [filename] [content]  | edit file, append content      |\n");
    printf("| wt       [filename] [content]  | edit file, cover content       |\n");
    printf("| del      [filename]            | delete file                    |\n");
    printf("| proc                           | show running process table     |\n");
    printf("| kill     [proc.pid]            | kill process                   |\n");
    printf("| pause    [proc.pid]            | pause process                  |\n");
    printf("| resume   [proc.pid]            | resume process                 |\n");
    printf("\n");
    printf(" Applications: tictactoe,boom\n");
}

/*Init the currentFold array*/
void initFolder() {
	for (int i = 1; i < 128; i++) {
		currentFolder[i] = '\0';
	}
}

/* Init FS */
void initFs()
{
    int fd = -1, n = 0, i = 0, count = 0, k = 0;
    char bufr[1024] = "";
    char bufp[1024] = "";
    char buff[1024] = "";

    for (i = 0; i < 500; i++)
        filequeue[i] = 1;


    initFolder();


    fd = open("myUsers", O_RDWR);
    close(fd);
    fd = open("myUsersPassword", O_RDWR);
    close(fd);
    fd = open("fileLogs", O_RDWR);
    close(fd);
    fd = open("user1", O_RDWR);
    close(fd);
    fd = open("user2", O_RDWR);
    close(fd);


    /* init users */
    fd = open("myUsers", O_RDWR);
    n = read(fd, bufr, 1024);
    bufr[strlen(bufr)] = '\0';


    for (i = 0; i < strlen(bufr); i++)
    {
        if (bufr[i] != ' ')
        {
            users[count][k] = bufr[i];
            k++;
        }
        else
        {
            while (bufr[i] == ' ')
            {
                i++;
                if (bufr[i] == '\0')
                {
                    users[count][k] = '\0';
                    if (strcmp(users[count], "empty") != 0)
                        usercount++;
                    count++;
                    break;
                }
            }
            if (bufr[i] == '\0')
            {
                break;
            }
            i--;
            users[count][k] = '\0';
            if (strcmp(users[count], "empty") != 0)
                usercount++;
            k = 0;
            count++;
        }
    }
    close(fd);
    count = 0;
    k = 0;


    /* init password */
    fd = open("myUsersPassword", O_RDWR);
    n = read(fd, bufp, 1024);
    for (i = 0; i < strlen(bufp); i++)
    {
        if (bufp[i] != ' ')
        {
            passwords[count][k] = bufp[i];
            k++;
        }
        else
        {
            while (bufp[i] == ' ')
            {
                i++;
                if (bufp[i] == '\0')
                {
                    count++;
                    break;
                }
            }
            if (bufp[i] == '\0')
                break;
            i--;
            passwords[count][k] = '\0';
            k = 0;
            count++;
        }
    }
    close(fd);
    count = 0;
    k = 0;

    /* init files */
    fd = open("fileLogs", O_RDWR);
    n = read(fd, buff, 1024);
    for (i = 0; i <= strlen(buff); i++)
    {
        if (buff[i] != ' ')
        {
            files[count][k] = buff[i];
            k++;
        }
        else
        {
            while (buff[i] == ' ')
            {
                i++;
                if (buff[i] == '\0')
                {
                    break;
                }
            }
            if (buff[i] == '\0')
            {
                files[count][k] = '\0';
                count++;
                break;
            }
            i--;
            files[count][k] = '\0';
            k = 0;
            count++;
        }
    }
    close(fd);

    int empty = 0;
    for (i = 0; i < count; i++)
    {
        char flag[7];
        strcpy(flag, "empty");
        flag[5] = '0' + i;
        flag[6] = '\0';
        fd = open(files[i], O_RDWR);
        close(fd);

        if (strcmp(files[i], flag) != 0)
            filequeue[i] = 0;
        else
            empty++;
    }
    filecount = count - empty;
}

/*Create folder*/
void createFolder(char* filepath,int flag) 
{
	int fd = -1, i = 0, pos;
	pos = getPos();
	char f[7];
	strcpy(f, "empty");
	f[5] = '0' + pos;
	f[6] = '\0';
	if (strcmp(files[pos], f) == 0 && flag == 1)
	{
		unlink(files[pos]);
	}

	fd = open(filepath, O_CREAT | O_RDWR);
	printf("folder name: %s \n", filepath);
	if (fd == -1)
	{
		printf("Operation failed, please check the path and try again!\n");
		return;
	}
	if (fd == -2)
	{
		printf("Operation failed, file already exists!\n");
		return;
	}
	close(fd);

	/* add log */
	if (flag == 1)
		addLog(filepath);
}

/* old Create File */
// void old_createFile(char *filepath, char *buf, int flag)
// {
//     int fd = -1, i = 0, pos;
//     pos = getPos();
//     char f[7];
//     strcpy(f, "empty");
//     f[5] = '0' + pos;
//     f[6] = '\0';
//     if (strcmp(files[pos], f) == 0 && flag == 1)
//     {
//         unlink(files[pos]);
//     }

//     fd = open(filepath, O_CREAT | O_RDWR);
//     printf("file name: %s content: %s\n", filepath, buf);
//     if (fd == -1)
//     {
//         printf("Fail, please check and try again!!\n");
//         return;
//     }
//     if (fd == -2)
//     {
//         printf("Fail, file exsists!!\n");
//         return;
//     }
//     //printf("%s\n", buf);

//     write(fd, buf, strlen(buf));
//     close(fd);

//     /* add log */
//     if (flag == 1)
//         addLog(filepath);
// }

void CreateFile(char* path, char* file)
{
    char absoPath[512];
    convert_to_absolute(absoPath, path, file);

    int fd = open(absoPath, O_CREAT | O_RDWR);

    if (fd == -1)
    {
        printf("Failed to create a new file with name %s\n", file);
        return;
    }

    char buf[1] = {0};
    write(fd, buf, 1);
    printf("File created: %s (fd %d)\n", file, fd);
    close(fd);
}

/*Get into the fold*/
void openFolder(char* filepath,char* filename)
{
	//Check if it is a back father path command
	if (strcmp(filename, "..") == 0) 
	{	
		int i = strlen(currentFolder) - 1,j;

		currentFolder[i] = '\0';
		for (j = i-1; j >= 0; j--) 
		{
			if (currentFolder[j] == '|')
				break;
		}
		j++;
		for (j; j < i; j++) 
		{
			currentFolder[j] = '\0';
		}
		printf("%s\n",currentFolder);
		return;
	}

	int name_length = strlen(filename);
	//Check if it is a folder
	if (filename[name_length-1] != '*')
		return;
	
	int i = 0,j = 0;

	for (i; i < 128; i++)
	{
		if (currentFolder[i] == '\0')
			break;
	}

	for (j = 0; j < strlen(filename); j++,i++)
	{
		currentFolder[i] = filename[j];
	}
	currentFolder[i++] = '|';
	currentFolder[i] = '\0';
}

/* Read File */
void readFile(char *filepath)
{
    if (vertify() == 0)
        return;

    int fd = -1;
    int n;
    char bufr[1024] = "";
    fd = open(filepath, O_RDWR);
    if (fd == -1)
    {
        printf("Fail, please check and try again!!\n");
        return;
    }
    n = read(fd, bufr, 1024);
    bufr[n] = '\0';
    printf("%s(fd=%d) : %s\n", filepath, fd, bufr);
    close(fd);
}

/* Edit File Appand */
void editAppand(char *filepath, char *buf)
{
    if (vertify() == 0)
        return;

    int fd = -1;
    int n, i = 0;
    char bufr[1024] = "";
    char empty[1024];

    for (i = 0; i < 1024; i++)
        empty[i] = '\0';
    fd = open(filepath, O_RDWR);
    if (fd == -1)
    {
        printf("Fail, please check and try again!!\n");
        return;
    }

    n = read(fd, bufr, 1024);
    n = strlen(bufr);

    for (i = 0; i < strlen(buf); i++, n++)
    {
        bufr[n] = buf[i];
        bufr[n + 1] = '\0';
    }
    write(fd, empty, 1024);
    fd = open(filepath, O_RDWR);
    write(fd, bufr, strlen(bufr));
    close(fd);
}

/* new Edit File Appand */
void new_editAppand(char *path, char *file, char *buf)
{
    char absoPath[512];
    convert_to_absolute(absoPath, path, file);

    if (vertify() == 0)
        return;

    int fd = -1;
    int n, i = 0;
    char bufr[1024] = "";
    char empty[1024];

    for (i = 0; i < 1024; i++)
        empty[i] = '\0';
    fd = open(absoPath, O_RDWR);
    if (fd == -1)
    {
        printf("Fail, please check and try again!!\n");
        return;
    }

    n = read(fd, bufr, 1024);
    n = strlen(bufr);

    for (i = 0; i < strlen(buf); i++, n++)
    {
        bufr[n] = buf[i];
        bufr[n + 1] = '\0';
    }
    write(fd, empty, 1024);
    close(fd);
    fd = open(absoPath, O_RDWR);
    write(fd, bufr, strlen(bufr));
    close(fd);
}

/* Edit File Cover */
void editCover(char *filepath, char *buf)
{

    if (vertify() == 0)
        return;

    int fd = -1;
    int n, i = 0;
    char bufr[1024] = "";
    char empty[1024];

    for (i = 0; i < 1024; i++)
        empty[i] = '\0';

    fd = open(filepath, O_RDWR);
    printf("%d\n",fd);
    if (fd == -1)
        return;
    write(fd, empty, 1024);
    close(fd);
    fd = open(filepath, O_RDWR);
    write(fd, buf, strlen(buf));
    close(fd);
}

/* new Edit File Cover */
void new_editCover(char *path, char *file, char *buf)
{
    char absoPath[512];
    convert_to_absolute(absoPath, path, file);

    if (vertify() == 0)
        return;

    int fd = -1;
    int n, i = 0;
    char bufr[1024] = "";
    char empty[1024];

    for (i = 0; i < 1024; i++)
        empty[i] = '\0';

    fd = open(absoPath, O_RDWR);
    // printf("%d\n",fd);
    if (fd == -1)
        return;
    write(fd, empty, 1024);
    close(fd);
    fd = open(absoPath, O_RDWR);
    write(fd, buf, strlen(buf));
    close(fd);
}

/* Delete File */
void deleteFile(char *filepath)
{
    if (vertify() == 0)
        return;
    if (usercount == 0)
    {
        printf("Fail!\n");
        return;
    }
    editCover(filepath, "");
    //printf("%s",filepath);
    int a = unlink(filepath);
    if (a != 0)
    {
        printf("Edit fail, please try again!\n");
        return;
    }
    deleteLog(filepath);

    char username[128];
    if (strcmp(currentUser, users[0]) == 0)
    {
        strcpy(username, "user1");
    }
    if (strcmp(currentUser, users[1]) == 0)
    {
        strcpy(username, "user2");
    }

    char userfiles[20][128];
    char bufr[1024];
    char filename[128];
    char realname[128];
    int fd = -1, n = 0, i = 0, count = 0, k = 0;
    fd = open(username, O_RDWR);
    n = read(fd, bufr, 1024);
    close(fd);

    for (i = strlen(currentUser) + 1; i < strlen(filepath); i++, k++)
    {
        realname[k] = filepath[i];
    }
    realname[k] = '\0';
    k = 0;
    for (i = 0; i < strlen(bufr); i++)
    {
        if (bufr[i] != ' ')
        {
            filename[k] = bufr[i];
            k++;
        }
        else
        {
            filename[k] = '\0';
            if (strcmp(filename, realname) == 0)
            {
                k = 0;
                continue;
            }
            strcpy(userfiles[count], filename);
            count++;
            k = 0;
        }
    }

    i = 0, k = 0;
    for (k = 0; k < 2; k++)
    {
        printf("%s\n", userfiles[k]);
    }
    editCover(username, "");
    while (i < count)
    {
        if (strlen(userfiles[i]) < 1)
        {
            i++;
            continue;
        }
        char user[128];
        int len = strlen(userfiles[i]);
        strcpy(user, userfiles[i]);
        user[len] = ' ';
        user[len + 1] = '\0';
        editAppand(username, user);
        i++;
    }
}

void shift(char *username, char *password)
{
    int i = 0;
    for (i = 0; i < usercount; i++)
    {
        if (strcmp(username, users[i]) == 0 && strcmp(password, passwords[i]) == 0 && strcmp(username, "empty") != 0)
        {
            strcpy(currentUser, users[i]);
            UserState = i + 1;
            printf("Welcome! %s!\n", users[i]);
            return;
        }
        //printf("%s %s %s %s",username,password,users[i],passwords[i]);
    }
    printf("Sorry! No such user!\n");
}

/* Add User */
void addUser(char *username, char *password)
{
    if (UserState == 3)
    {
        int i;
        for (i = 0; i < 2; i++)
        {
            if (strcmp(users[i], username) == 0)
            {
                printf("User exists!\n");
                return;
            }
        }
        if (usercount == 2)
        {
            printf("No more users\n");
            return;
        }
        if (strcmp(users[0], "empty") == 0)
        {
            strcpy(users[0], username);
            strcpy(passwords[0], password);
            usercount++;
            updateMyUsers();
            updateMyUsersPassword();
            return;
        }
        if (strcmp(users[1], "empty") == 0)
        {
            strcpy(users[1], username);
            strcpy(passwords[1], password);
            usercount++;
            updateMyUsers();
            updateMyUsersPassword();
            return;
        }
    }
    else
        printf("Permission Deny!");
}

/* Move User */
void moveUser(char *username, char *password)
{
    if (UserState == 3)
    {
        int i = 0;
       printf("%s\n%s", users[0], users[1]);
        for (i = 0; i < 2; i++)
        {
            if (strcmp(username, users[i]) == 0 && strcmp(password, passwords[i]) == 0)
            {
                //strcpy(currentUser, username);
                int fd = -1, n = 0, k = 0, count = 0;
                char bufr[1024], deletefile[128];
                if (i == 0)
                {
                    fd = open("user1", O_RDWR);
                }
                if (i == 1)
                {
                    fd = open("user2", O_RDWR);
                }
                n = read(fd, bufr, 1024);
                close(fd);
                for (k = 0; k < strlen(bufr); k++)
                {
                    if (bufr[k] != ' ')
                    {
                        deletefile[count] = bufr[k];
                        count++;
                    }
                    else
                    {
                       // deletefile[count] = '\0';
                       // createFilepath(deletefile);
                      //  deleteFile(filepath);
                        count = 0;
                    }
                }
                printf("Delete %s!\n", users[i]);
                strcpy(users[i], "empty");
                strcpy(passwords[i], "");
                updateMyUsers();
                updateMyUsersPassword();
                usercount--;
                strcpy(currentUser, "/");
                return;
            }
        }
        printf("Sorry! No such user!\n");
    }
    else
        printf("Permission Deny!");
}
/* Compare the file path to current directory */
void pathCompare(char* temp)
{
	char current_temp[256];
	char filename_only[64];
	int i = 0,j = 0,k = 0;
	int flag =1;

	for(i;i < strlen(currentFolder);i++)
	{
		current_temp[i] = currentFolder[i + 1];	
	}
	
	for(j;j < strlen(current_temp);j++)
	{
		if(current_temp[j] != temp[j])
		{
			flag = 0;
			break;
		}	
	}
	if(flag == 1)
	{
        // in our "multi-class" file system, the sub-directory should not be shown
        // so u need to do sth...
		for(j; j < strlen(temp)&&temp[j]!='|';j++,k++)
		{
			filename_only[k] = temp[j];
		}
		filename_only[k] = '\0';

		printf("%s\n",filename_only);	
	}
}

/* split path */
void pathFilter(char* bufr)
{
	char *p,*q;
	char temp[128];
	int length = 0;
	p = bufr;
	q = p;
	while(1)
	{
		while(*q != ' ' && *q != '\0')
		{
			q++;												
		}
		if(*q == '\0'){
            return;
        }
		else{
			*q = '\0';
			length = q - p;
			int i = 0;
			for(i;i < length; i++,p++)
			{
				temp[i] = *p;
			}
			temp[i] = '\0';
			//printf("temp=%s\n",temp);
			pathCompare(temp);
			q++;
			p = q;
		}
	}
}

/* old Ls */
void old_ls()
{
	int fd = -1, n;
	char bufr[1024];
	if (strcmp(currentUser, users[0]) == 0)
	{
		fd = open("user1", O_RDWR);
		if (fd == -1)
		{
			printf("empty\n");
		}
		n = read(fd, bufr, 1024);
		pathFilter(bufr);												
		//printf("%s\n", bufr);
		close(fd);
	}
	else if(strcmp(currentUser, users[1]) == 0)
	{
		fd = open("user2", O_RDWR);
		if (fd == -1)
		{
			printf("empty\n");
		}
		n = read(fd, bufr, 1024);
		pathFilter(bufr);
		//printf("%s\n", bufr);
		close(fd);
	}
	else
		printf("Permission deny!\n");
}

/* ls */
int ls(char* pathName)  // 传入当前目录，发送当前目录下的文件名
{
    MESSAGE msg;
    msg.type = LS;  // ls类型的消息（这个说法怪怪的）

    msg.PATHNAME = (void*)pathName;
    msg.NAME_LEN = strlen(pathName);
    msg.FLAGS = 0;

    send_recv(BOTH, TASK_FS, &msg);

    return msg.RETVAL;
}

/* Show Process */
void showProcess()
{	
    // printf("xm\n");
    int i = 0;
	printf("---------------------------------");
    printf(POLICY_MULTI_QUE==get_schd_policy()?"---------\n":"\n");
    printf("| pid |    name     |   state   |");
    printf(POLICY_MULTI_QUE==get_schd_policy()?"  rank |\n":"\n");
    printf("---------------------------------");
    printf(POLICY_MULTI_QUE==get_schd_policy()?"---------\n":"\n");
	for (i = 0; i < NR_TASKS + NR_NATIVE_PROCS; i++)
	{
		if(proc_table[i].p_flags != 1){
			printf("|  %d",proc_table[i].pid);
			if(proc_table[i].pid<10) printf("  ");
			else if(proc_table[i].pid<100) printf(" ");

        		printf("| %s",proc_table[i].name);
			int j;
        		for(j=0;j<12-len(proc_table[i].name);j++)
            			printf(" ");

			if(proc_table[i].run_state) printf("|  running  ");
			else printf("|  paused  ");

            if(POLICY_MULTI_QUE==get_schd_policy()){
                printf("   %d   ", proc_table[i].rank);
            }

			if(i <= NR_TASKS) printf("-");
			else printf(" ");
        		printf("|\n");	
		}
	}
	printf("---------------------------------");
    printf(POLICY_MULTI_QUE==get_schd_policy()?"---------\n":"\n");
}

/*****************************************************************************
 *                                Init
 *****************************************************************************/
/**
 * The hen.
 * 
 *****************************************************************************/
void Init()
{
    int fd_stdin = open("/dev_tty0", O_RDWR);
    assert(fd_stdin == 0);
    int fd_stdout = open("/dev_tty0", O_RDWR);
    assert(fd_stdout == 1);

    printf("Init() is running ...\n");

    /* extract `cmd.tar' */
    // untar("/cmd.tar");
    welcomeAnimation();
    welcome();
    //shabby_shell("/dev_tty0");

    //char * tty_list[] = {"/dev_tty1", "/dev_tty2"};
    char *tty_list[] = {"/dev_tty0", "/dev_tty1", "/dev_tty2"};

    int i;
    for (i = 0; i < sizeof(tty_list) / sizeof(tty_list[0]); i++)
    {
        int pid = fork();
        if (pid != 0)
        {   /* parent process */
            //printf("[parent is running, child pid:%d]\n", pid);
        }
        else
        { /* child process */
            //printf("[child is running, pid:%d]\n", getpid());
            close(fd_stdin);
            close(fd_stdout);
            shabby_shell(tty_list[i]);
            assert(0);
        }
    }

    while (1)
    {
        int s;
        int child = wait(&s);
        printf("child (%d) exited with status: %d.\n", child, s);
    }
    assert(0);
}

/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
    while (1)
    {
        if (proc_table[6].run_state == 1)
        {
        }
    }
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
    while (1)
    {
        if (proc_table[7].run_state == 1)
        {
        }
    }
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestC()
{
    while (1)
    {
        if (proc_table[8].run_state == 1)
        {
        }
    }
}

/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
    int i;
    char buf[256];

    /* 4 is the size of fmt in the stack */
    va_list arg = (va_list)((char *)&fmt + 4);

    i = vsprintf(buf, fmt, arg);

    printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

    /* should never arrive here */
    __asm__ __volatile__("ud2");
}

void GoDir(char* path, char* file)
{
    int flag = 0;  // 判断是进入下一级目录还是返回上一级目录
    char newPath[512] = {0};
    if (file[0] == '.' && file[1] == '.')  // cd ..返回上一级目录
    {
        flag = 1;
        int pos_path = 0;
        int pos_new = 0;
        int i = 0;
        char temp[128] = {0};  // 用于存放某一级目录的名称
        while (path[pos_path] != 0)
        {
            if (path[pos_path] == '/')
            {
                pos_path++;
                if (path[pos_path] == 0)  // 已到达结尾
                    break;
                else
                {
                    temp[i] = '/';
                    temp[i + 1] = 0;
                    i = 0;
                    while (temp[i] != 0)
                    {
                        newPath[pos_new] = temp[i];
                        temp[i] = 0;  // 抹掉
                        pos_new++;
                        i++;
                    }
                    i = 0;
                }
            }
            else
            {
                temp[i] = path[pos_path];
                i++;
                pos_path++;
            }
        }
    }
    char absoPath[512];
    char temp[512];
    int pos = 0;
    while (file[pos] != 0)
    {
        temp[pos] = file[pos];
        pos++;
    }
    temp[pos] = '/';
    temp[pos + 1] = 0;
    if (flag == 1)  // 返回上一级目录
    {
        temp[0] = 0;
        convert_to_absolute(absoPath, newPath, temp);
    }
    else  // 进入下一级目录
        convert_to_absolute(absoPath, path, temp);

    // printf("%s\n", absoPath);

    int fd = open(absoPath, O_RDWR);
    if (fd == -1)
        printf("%s is not a directory!\n", absoPath);
    else
    {
        memcpy(path, absoPath, 512);
        close(fd);
    }
        
}


void DeleteFile(char* path, char* file)
{
    char absoPath[512];
    convert_to_absolute(absoPath, path, file);
    int m=unlink(absoPath);
    if (m == 0)
        printf("%s deleted!\n", file);
    else
        printf("Failed to delete %s!\n", file);
}

void ReadFile(char* path, char* file)
{
    char absoPath[512];
    convert_to_absolute(absoPath, path, file);
    int fd = open(absoPath, O_RDWR);
    if (fd == -1)
    {
        printf("Failed to open %s!\n", file);
        return;
    }

    char buf[4096];
    int n = read(fd, buf, 4096);
    if (n == -1)  // 读取文件内容失败
    {
        printf("An error has occured in reading the file!\n");
        close(fd);
        return;
    }

    printf("%s\n", buf);
    close(fd);
}

int textPad(char* path, char *file){
    int fd = 0;
    int n;
    char text[1024] = "";
    char pr[20][128];
    char absoPath[512];
    convert_to_absolute(absoPath, path, file);
    fd = open(absoPath, O_RDWR);

    if(fd == -1)
    {
        printf("failed to open file\n");
        return 0;
    }

    n = read(fd, text, 1024);
    text[n] = '\0';
    close(fd);

    int i = 0, j = 0, k = 0;
    while(i <= n)
    {
        if(text[i] <= 31 && j != 0)
        {
            pr[k][j] = '\0';
            j = 0;
            k++;
        }
        else if(text[i] > 31)
        {
            pr[k][j] = text[i];
            j++;
        }
        i++;
    }

    showPr(k, pr);

        /* display */
    // printf("*********************** textPad ***********************\n");
    // printf("      %s                       help: guidance\n",filepath);
    // printf("=======================================================\n");

    // int mn = 0;
    // if(k > 9)
    //     mn = 9;
    // else
    //     mn = k;
    // for(i = 0; i < k; i++)
    // {
    //     printf("[%d] %s\n", i, pr[i]);
    // }
    // if(k > 99999)
    // {
    //     for(i = 10; i < k; i++)
    //         printf("[%d] %s\n", i, pr[i]);
    // }

    while(1)
    {
        char rdbuf[75];
        char copy[70];
        int r = read(0, rdbuf, 75);
        rdbuf[r] = 0;
        char cmd[10] = "";
        int loc = 0;
        char arg[70] = "";
        if(rdbuf[0]){
            i = 0, j = 0, loc = 0;
            while(rdbuf[i] != ' ' && rdbuf[i] != 0)
            {
                cmd[i] = rdbuf[i];
                i++;
            }
            i++;

            while(rdbuf[i] >= '0' && rdbuf[i] <= '9')
            {
                loc = loc * 10 + rdbuf[i] - '0';
                i++;
                j++;
            }
            if(loc < 0 || loc > k){
                printf("Wrong line number %d. It is unaccessable.\n", loc);
                continue;

            }
            if(rdbuf[i] == ' ')
                i++;
            j = 0;
            while(rdbuf[i] != 0)
            {
                arg[j] = rdbuf[i];
                i++;
                j++;
            }

            if(strcmp(cmd, "edit") == 0){
                strcpy(pr[loc], arg);

                printf("edit successfully!\n");
                printf("%s\n", pr[loc]);
                showPr(k, pr);

            }
             else if(strcmp(cmd, "append") == 0){

                k++;
                printf("k = %d\n", k);
                strcpy(pr[k], arg);
                printf("append successfully!\n");
                showPr(k, pr);
            }

            else if(strcmp(cmd, "clear") == 0)
            {
                printf("this is clear\n");
                for(int i = 0; i <= k; i++)
                {
                    strcpy(pr[i], "");
                }
                k = 0;
                printf("clear successfully!]\n");
                showPr(k, pr);
            }
            else if(strcmp(cmd, "insert") == 0){
                //for(i = k - 1; i > loc; i--)
                    //strcpy(pr[i + 1], pr[i]);
                strcat(pr[loc], arg);
               // k++;
                printf("insert successfully!\n");
                       /* display */
                showPr(k, pr);
            }
            else if(strcmp(cmd, "delete") == 0)
            {
                strcpy(pr[loc], "");
                for(i = loc + 1; i < k; i++)
                {
                    strcpy(pr[i - 1], pr[i]);
                }
                printf("delete successfully\n");
                k--;
                showPr(k, pr);
            }
            else if(strcmp(cmd, "copy") == 0)
            {
                strcpy(copy, pr[loc]);
                printf("line %d is in the clipboard\n", loc);
                continue;
            }
            else if(strcmp(cmd, "cut") == 0)
            {
                strcpy(copy, pr[loc]);
                strcpy(pr[loc], "");
                // for(i = loc + 2; i <= k; i++)
                //     strcpy(pr[i-1],pr[i]);
                printf("line %d is in the clipboard!\n", loc);
               // k--;
                showPr(k, pr);
            }
            else if(strcmp(cmd, "clipboard") == 0)
            {
                printf("[CLIPBOARD: %s\n", copy);
                continue;
            }
            else if(strcmp(cmd, "paste") == 0){
                for(i = k; i >= loc; i--)
                    strcpy(pr[i + 1], pr[i]);
                strcpy(pr[loc],copy);
                k++;
                printf("paste successfully\n");
                showPr(k, pr);
            }

            else if(strcmp(cmd, "save") == 0)
            {
                new_editCover(current_dirr, file, "");
                for(i = 0; i <= k; i++)
                {
                    new_editAppand(current_dirr, file, pr[i]);
                    new_editAppand(current_dirr,file, "\n");
                }
                printf("save successfully\n");
                return 0;
            }


                else if (strcmp(cmd, "exit") == 0 ){
                    return 0;
                }
                else if(strcmp(cmd, "help") == 0)
                {
                        printf("======================================================\n");
                        printf("  insert [line] [text] | insert a line.\n");
                        printf("  edit [line] [text]   | reverse a line.\n");
                        printf("  append [text]        | append a line at the end.\n");
                        printf("  delete [line]        | delete a line.\n");
                        printf("  clear                | delete all texts.\n");
                        printf("  copy [line]          | copy a line into clipboard.\n");
                        printf("  cut [line]           | delete a line after copying.\n");
                        printf("  paste [line]         | insert a line from clipboard.\n");
                        printf("  clipboard            | print the clipboard.\n");
                        printf("  help                 | print a guidance of textPad.\n");
                        printf("-----------------------|------------------------------\n");
                        printf("  save                 | exit and save the texts.\n");
                        printf("  exit                 | just exit textPad.\n");
                        printf("======================================================\n");
                        continue;
                }

                    }
                }
    return 0;


}

void showPr(int k, char (*pr)[128])
{
    printf("*********************** textPad ***********************\n");
    printf("      %s                       help: guidance\n",filepath);
    printf("=======================================================\n");

    int mn = 0;
    if(k > 9)
        mn = 9;
    else
        mn = k;
    for(int i = 0; i <= k; i++)
    {
        printf("[%d] %s\n", i, pr[i]);
    }
    if(k > 99999)
    {
        for(int i = 10; i < k; i++)
            printf("[%d] %s\n", i, pr[i]);
    }

}

int my_date()
{
    struct time t;
    date(&t);
    printf("%d-%d-%dT%d:%d:%d\n", t.year, t.month, t.day, t.hour, t.minute, t.second);
    return 0;
}

