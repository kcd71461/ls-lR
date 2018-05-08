#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))

#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <malloc.h>
#include <pwd.h>
#include <time.h>
#include <memory.h>
#include <grp.h>

/**
 * Directory내 entry,stat 들을 저장해놓기 위한 linked list의 node
 */
typedef struct FileInfoListNode {
    struct stat *pFileStat;
    struct dirent *pDirectoryEntry;
    struct FileInfoListNode *next;
} FileInfoListNode;

/**
 * 파일 정보를 출력할때 각 필드의 길이 값이 필요해서 그 길이들의 max만 담는 구조체
 */
typedef struct MaxLengthInfo {
    int nLinkLen, userLen, groupLen, sizeLen;
} MaxLengthInfo;

/**
 * startNode가 바뀌면 true를 리턴함, list에 이름 오름차순으로 추가됨
 * @param pStatNode
 * @param appendNode
 * @return
 */
bool statListAppend(FileInfoListNode *pStatNode, FileInfoListNode *appendNode) {
    if (pStatNode == NULL) {
        return false;
    } else {
        FileInfoListNode *prevNode = NULL;
        while (pStatNode != NULL) {
            if (strcmp(appendNode->pDirectoryEntry->d_name, pStatNode->pDirectoryEntry->d_name) < 0) {
                if (prevNode != NULL) {
                    prevNode->next = appendNode;
                }
                appendNode->next = pStatNode;
                return prevNode == NULL;
            }
            prevNode = pStatNode;
            pStatNode = pStatNode->next;
        }
        prevNode->next = appendNode;
    }
    return false;
}

/**
 * 파일 리스트를 출력할때 각 필드의 max length를 계산
 * @param pNode
 * @param maxLengthInfo
 */
void computeFieldMaxLengths(FileInfoListNode *pNode, MaxLengthInfo *maxLengthInfo);

void printDirectoryRecursively(char *pathname);

void printFileInfo(FileInfoListNode *pNode, MaxLengthInfo *pMaxLengthInfo);

int main() {
    printDirectoryRecursively(".");
    return (0);
}

/**
 * 10진수 몇자리인지를 반환
 * @param i
 * @return
 */
int nDigits(unsigned i) {
    int n = 1;
    while (i > 9) {
        n++;
        i /= 10;
    }
    return n;
}

/**
 * FileInfoList를 모드 해제
 * @param pNode
 */
void freeAllNodes(FileInfoListNode *pNode) {
    if (pNode != NULL) {
        freeAllNodes(pNode->next);
        free(pNode->pFileStat);
        free(pNode);
    }
}

/**
 * 두개의 path를 combine
 * @param path1
 * @param path2
 * @return
 */
char *pathCombine(char *path1, char *path2) {
    char *newPath = (char *) malloc((strlen(path1) + strlen(path2) + 2) * sizeof(char));
    sprintf(newPath, "%s/%s", path1, path2);
    return newPath;
}

/**
 * 재귀적으로 디렉토리들을 탐색하며 정보를 출력
 * @param pathname
 */
void printDirectoryRecursively(char *pathname) {
    printf("%s:\n", pathname); // 현재 조회 디렉토리명 출력

    long total = 0;
    DIR *d;
    struct dirent *dir;
    FileInfoListNode *startNode = NULL;
    d = opendir(pathname);

    MaxLengthInfo maxLengthInfo;
    maxLengthInfo.nLinkLen = maxLengthInfo.userLen = maxLengthInfo.groupLen = maxLengthInfo.sizeLen = 0;

    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (dir->d_name[0] == '.') {
                continue;
            }
            struct stat *pStatbuf = (struct stat *) malloc(sizeof(struct stat));
            char *filePath = pathCombine(pathname, dir->d_name);
            if (stat(filePath, pStatbuf) == 0) {
                total += pStatbuf->st_blocks;

                FileInfoListNode *newNode = (FileInfoListNode *) malloc(sizeof(FileInfoListNode));
                newNode->pFileStat = pStatbuf;
                newNode->pDirectoryEntry = dir;
                newNode->next = NULL;
                computeFieldMaxLengths(newNode, &maxLengthInfo);

                if (startNode == NULL) {
                    startNode = newNode;
                } else {
                    if (statListAppend(startNode, newNode)) {
                        startNode = newNode;
                    }
                }
            }
            free(filePath);
        }

        printf("합계 %lu\n", total / 2); // print 합계:

        if (startNode != NULL) {
            FileInfoListNode *loopingNode = startNode;
            while (loopingNode != NULL) {
                printFileInfo(loopingNode, &maxLengthInfo);
                loopingNode = loopingNode->next;
            }
            loopingNode = startNode;
            while (loopingNode != NULL) {
                if (loopingNode->pDirectoryEntry->d_type == DT_DIR) {
                    printf("\n");
                    char *newPath = pathCombine(pathname, loopingNode->pDirectoryEntry->d_name);
                    printDirectoryRecursively(newPath);
                    free(newPath);
                }
                loopingNode = loopingNode->next;
            }
        }
        freeAllNodes(startNode);
        closedir(d);
    }
}

/**
 * 권한 6자리를 출력
 * @param mod
 */
void printOwnership(__mode_t mod) {
    if (mod & S_IRUSR) {
        printf("r");
    } else {
        printf("-");
    }
    if (mod & S_IWUSR) {
        printf("w");
    } else {
        printf("-");
    }
    if (mod & S_IXUSR) {
        printf("x");
    } else {
        printf("-");
    }
    if (mod & S_IRGRP) {
        printf("r");
    } else {
        printf("-");
    }
    if (mod & S_IWGRP) {
        printf("w");
    } else {
        printf("-");
    }
    if (mod & S_IXGRP) {
        printf("x");
    } else {
        printf("-");
    }
    if (mod & S_IROTH) {
        printf("r");
    } else {
        printf("-");
    }
    if (mod & S_IWOTH) {
        printf("w");
    } else {
        printf("-");
    }
    if (mod & S_IXOTH) {
        printf("x");
    } else {
        printf("-");
    }
}

/**
 * 시간 정보를 출력
 * @param sec
 */
void printTime(__time_t sec) {
    char date[17];
    strftime(date, 17, "%Y-%m-%d %H:%M", localtime(&sec));
    printf("%s", date);
}

/**
 * uid로 부터 username을 가져온다
 * @param uid
 * @return
 */
char *getUserName(__uid_t uid) {
    struct passwd *pPw = getpwuid(uid);
    return pPw->pw_name;
}

/**
 * gid로부터 groupname을 가져온다
 * @param gid
 * @return
 */
char *getGroupName(__uid_t gid) {
    struct group *pGroup = getgrgid(gid);
    return pGroup->gr_name;
}

/**
 * file stat field의 max length들을 조정한다
 * @param pNode
 * @param maxLengthInfo
 */
void computeFieldMaxLengths(FileInfoListNode *pNode, MaxLengthInfo *maxLengthInfo) {
    maxLengthInfo->nLinkLen = MAX(maxLengthInfo->nLinkLen, nDigits(pNode->pFileStat->st_nlink));
    maxLengthInfo->sizeLen = MAX(maxLengthInfo->sizeLen, nDigits(pNode->pFileStat->st_size));
    maxLengthInfo->userLen = MAX(maxLengthInfo->userLen, strlen(getUserName(pNode->pFileStat->st_uid)));
    maxLengthInfo->groupLen = MAX(maxLengthInfo->groupLen, strlen(getGroupName(pNode->pFileStat->st_gid)));
}

/**
 * 최소 길이값을 옵션으로 받는 number print 함수
 * @param nlink
 * @param len
 */
void printNumberWithMinLength(unsigned nlink, int len) {
    int i = 0;
    for (; i < (len - nDigits(nlink)); i++) {
        printf(" ");
    }
    printf("%u", nlink);
}

/**
 * 최소 길이값을 옵션으로 받는 string print 함수
 * @param name
 * @param len
 */
void printStringWithMinLength(char *name, int len) {
    int i = 0;
    for (; i < (len - strlen(name)); i++) {
        printf(" ");
    }
    printf("%s", name);
}


/**
 * 파일 정보를 출력
 * @param pNode
 * @param pMaxLengthInfo
 */
void printFileInfo(FileInfoListNode *pNode, MaxLengthInfo *pMaxLengthInfo) {
    switch (pNode->pDirectoryEntry->d_type) {
        case DT_FIFO:
            printf("p");
            break;
        case DT_CHR:
            printf("c");
            break;
        case DT_DIR:
            printf("d");
            break;
        case DT_BLK:
            printf("b");
            break;
        case DT_REG:
            printf("-");
            break;
        case DT_LNK:
            printf("l");
            break;
        case DT_SOCK:
            printf("s");
            break;
        default:
            return;
    }
    printOwnership(pNode->pFileStat->st_mode);
    printNumberWithMinLength(pNode->pFileStat->st_nlink, pMaxLengthInfo->nLinkLen + 1); // print hard link count

    printStringWithMinLength(getUserName(pNode->pFileStat->st_uid), pMaxLengthInfo->userLen + 1);
    printStringWithMinLength(getGroupName(pNode->pFileStat->st_gid), pMaxLengthInfo->groupLen + 1);

    printNumberWithMinLength(pNode->pFileStat->st_size, pMaxLengthInfo->sizeLen + 1); // print file size
    printf(" ");
    printTime(pNode->pFileStat->st_mtime);
    printf(" %s", pNode->pDirectoryEntry->d_name);
    printf("\n");
}