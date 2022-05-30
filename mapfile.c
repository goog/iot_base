#include <stdlib.h>
#include <string.h>
#include <stdio.h>
//#include "stdint.h"
#include <fcntl.h>

#include <sys/stat.h>
#include <unistd.h>
#include "mapfile.h"



static int read_kv_item(const char *filename, void *buf, int location);
static int write_kv_item(const char *filename, void *data, int location);



static void free_kv(struct kv *kv)
{
    if (kv) {
        kv->value_len = 0;
        free(kv);
    }
}

static unsigned int hash_gen(const char *key)
{
    unsigned int hash = 0;
    while (*key) {
        hash = (hash << 5) + hash + *key++;
    }
    return hash % TABLE_COL_SIZE;
}

/* insert or update a value indexed by key */
static int hash_table_put(kv_file_t *file, const  char *key, void *value, int value_len)
{
    int i;
    int read_size = 0;
    kv_item_t *kv = NULL;
    int j = 0;
    kv_item_t *p;
    if (!file || !file->filename ||  !key || !value  || value_len <= 0) {
        kv_err("paras err");
        return MAPFILE_FAIL;
    }

    value_len = value_len > ITEM_MAX_VAL_LEN ? ITEM_MAX_VAL_LEN : value_len;
    i = hash_gen(key);
    kv_err("hash i= %d", i);
    read_size = ITEM_MAX_LEN * TABLE_ROW_SIZE + 1;
    kv = malloc(read_size);
    if (kv == NULL) {
        kv_err("malloc kv err");
        return MAPFILE_FAIL;
    }

    memset(kv, 0, read_size);
    if (read_kv_item(file->filename, kv, i) != 0) {
        kv_err("read kv err");
        free_kv(kv);
        return MAPFILE_FAIL;
    }
    p = &kv[j];

    while (p && p->value_len) { 

        if (memcmp(p->key, key, strlen(key)) == 0) {
            memset(p->value, 0, ITEM_MAX_VAL_LEN);  /* if key is already stored, update its value */
            memcpy(p->value, value, value_len);
            p->value_len = value_len;
            break;
        }

        if (++j == TABLE_ROW_SIZE) {
            kv_err("hash row full");
            free(kv);
            return MAPFILE_FAIL;
        }
        p = &kv[j];
    }

    p = &kv[j];
    if (p && !p->value_len) {/* if key has not been stored, then add it */
        //p->next = NULL;
        strncpy(p->key, key, ITEM_MAX_KEY_LEN - 1);
        memcpy(p->value, value, value_len);
        p->value_len = value_len;
    }

    if (write_kv_item(file->filename, kv, i) < 0) {
        kv_err("write_kv_item err");
        free(kv);
        return MAPFILE_FAIL;
    }
    free(kv);
    return MAPFILE_OK;
}

/* get a value indexed by key */
static int hash_table_get(kv_file_t *file, const char *key, void *value, int *len)
{
    int i;
    int read_size;
    kv_item_t *kv;
    int j = 0;
    struct kv *p;
    if (!file || !file->filename || !key || !value || !len  || *len <= 0) {
        kv_err("paras err");
        return MAPFILE_FAIL;
    }

    i = hash_gen(key);

    read_size = ITEM_MAX_LEN * TABLE_ROW_SIZE + 1;
    kv = malloc(read_size);
    if (kv == NULL) {
        kv_err("malloc kv err");
        return MAPFILE_FAIL;
    }

    memset(kv, 0, read_size);
    if (read_kv_item(file->filename, kv, i) != 0) {
        kv_err("read kv err");
        free_kv(kv);
        return MAPFILE_FAIL;
    }

    // struct kv *p = ht->table[i];
    p = &kv[j];

    while (p && p->value_len) {
        if (memcmp(key, p->key, strlen(key)) == 0) {
            *len = p->value_len < *len ? p->value_len : *len;
            memcpy(value, p->value, *len);
            free_kv(kv);
            return MAPFILE_OK;
        }
        if (++j == TABLE_ROW_SIZE) {
            break;
        }
        p = &kv[j];
    }
    free_kv(kv);
    kv_err("kv key:%s not found", key);
    return MAPFILE_FAIL;
}

/* remove a value indexed by key */
static int hash_table_rm(kv_file_t *file,  const  char *key)
{
    int i;
    int read_size;
    kv_item_t *kv;
    int j = 0;
    struct kv *p;
    if (!file || !file->filename ||  !key) {
        return MAPFILE_FAIL;
    }
    i = hash_gen(key); //% TABLE_COL_SIZE;
    read_size = sizeof(kv_item_t) * TABLE_ROW_SIZE + 1;
    kv = malloc(read_size);
    if (kv == NULL) {
        return MAPFILE_FAIL;
    }

    memset(kv, 0, read_size);
    if (read_kv_item(file->filename, kv, i) != 0) {
        free_kv(kv);
        return MAPFILE_FAIL;
    }

    p = &kv[j];

    while (p && p->value_len) {
        if (memcmp(key, p->key, strlen(key)) == 0) {
            memset(p, 0, ITEM_MAX_LEN);
        }
        if (++j == TABLE_ROW_SIZE) {
            break;
        }
        p = &kv[j];
    }

    if (write_kv_item(file->filename, kv, i) < 0) {
        free_kv(kv);
        return MAPFILE_FAIL;
    }
    free_kv(kv);
    return MAPFILE_OK;
}

static int read_kv_item(const char *filename, void *buf, int location)
{
    struct stat st;
    int ret = 0;
    int offset;
    if (filename == NULL || buf == NULL) {
        kv_err("paras err");
        return MAPFILE_FAIL;
    }

    int fd = open(filename, O_RDONLY);

    if (fd < 0) {
        perror("open file fail !\n");
        kv_err("open %s err" , filename);
        return MAPFILE_FAIL;
    }

    if (fstat(fd, &st) < 0) {
        kv_err("fstat err");
        close(fd);
        return MAPFILE_FAIL;
    }

    if (st.st_size < (location + 1) *ITEM_MAX_LEN * TABLE_ROW_SIZE) {
        kv_err("read overstep");
        close(fd);
        return MAPFILE_FAIL;
    }

    offset =  location * ITEM_MAX_LEN * TABLE_ROW_SIZE;
    ret = lseek(fd, offset, SEEK_SET);
    if (ret < 0) {
        kv_err("lseek err");
        close(fd);
        return MAPFILE_FAIL;
    }

    if (read(fd, buf, ITEM_MAX_LEN * TABLE_ROW_SIZE) != ITEM_MAX_LEN * TABLE_ROW_SIZE) {
        kv_err("read err");
        close(fd);
        return MAPFILE_FAIL;
    }

    close(fd);
    return MAPFILE_OK;
}


static int read_kv_item_(int fd, void *buf, int location)
{
    struct stat st;
    int ret = 0;
    int offset;
    if (buf == NULL) {
        kv_err("paras err");
        return MAPFILE_FAIL;
    }

    lseek(fd, 0, SEEK_SET);
    //int fd = open(filename, O_RDONLY);

    if (fd < 0) {
        kv_err("open err");
        return MAPFILE_FAIL;
    }

    if (fstat(fd, &st) < 0) {
        kv_err("fstat err");
        //close(fd);
        return MAPFILE_FAIL;
    }

    if (st.st_size < (location + 1) *ITEM_MAX_LEN * TABLE_ROW_SIZE) {
        kv_err("read overstep");
        //close(fd);
        return MAPFILE_FAIL;
    }

    offset =  location * ITEM_MAX_LEN * TABLE_ROW_SIZE;
    ret = lseek(fd, offset, SEEK_SET);
    if (ret < 0) {
        kv_err("lseek err");
        //close(fd);
        return MAPFILE_FAIL;
    }

    if (read(fd, buf, ITEM_MAX_LEN * TABLE_ROW_SIZE) != ITEM_MAX_LEN * TABLE_ROW_SIZE) {
        kv_err("read err");
        //close(fd);
        return MAPFILE_FAIL;
    }

    //close(fd);
    return MAPFILE_OK;
}


int mapfile_kv_item_get(int fd, void *buf, int location)
{
    return read_kv_item_(fd, buf, location);
}

static int write_kv_item(const char *filename, void *data, int location)
{
    struct stat st;
    int offset;
    int ret;
    int fd = open(filename, O_WRONLY);
    if (fd < 0) {
        perror("open file fail !\n");
        kv_err("open %s err" , filename);
        return MAPFILE_FAIL;
    }

    if (fstat(fd, &st) < 0) {
        kv_err("fstat err");
        close(fd);
        return MAPFILE_FAIL;
    }

    if (st.st_size < (location + 1) *ITEM_MAX_LEN * TABLE_ROW_SIZE) {
        kv_err("overstep st.st_size = %d location =%d cur loc=%d",
               (int)st.st_size,
               (int)location,
               (int)((location + 1) *ITEM_MAX_LEN * TABLE_ROW_SIZE));
        close(fd);
        return MAPFILE_FAIL;
    }

    offset = (location) * ITEM_MAX_LEN * TABLE_ROW_SIZE;
    ret = lseek(fd, offset, SEEK_SET);
    if (ret < 0) {
        kv_err("lseek err");
        close(fd);
        return MAPFILE_FAIL;
    }

    if (write(fd, data, ITEM_MAX_LEN * TABLE_ROW_SIZE) != ITEM_MAX_LEN * TABLE_ROW_SIZE) {
        kv_err("kv write failed");
        close(fd);
        return MAPFILE_FAIL;
    }

    fsync(fd);
    close(fd);

    return MAPFILE_OK;
}

static int create_hash_file(kv_file_t *hash_kv)
{
    
    int fd;
    int ret = 0;
    
    if (hash_kv == NULL) {
        return MAPFILE_FAIL;
    }
    fd = open(hash_kv->filename, O_CREAT | O_RDWR, 0644); // future accesses -rw-r-r–
    if (fd < 0) {
        return MAPFILE_FAIL;
    }

    ret = ftruncate(fd, ITEM_MAX_LEN * TABLE_ROW_SIZE * TABLE_COL_SIZE);
    if(ret == -1)
    {
        kv_err("mapfile ftruncate failed");
        close(fd);
        return MAPFILE_FAIL;    
    }

    if(fsync(fd) < 0) 
    {
        close(fd);
        return MAPFILE_FAIL;
    }

    close(fd);
    return MAPFILE_OK;
}


kv_file_t *mapfile_kv_open(const char *filename)
{
    int size = 0;
    struct stat st;
    
    kv_file_t *file = malloc(sizeof(kv_file_t));
    if (!file) {
        return NULL;
    }
    memset(file, 0, sizeof(kv_file_t));

    file->filename = filename;
    pthread_mutex_init(&file->lock, NULL);
    pthread_mutex_lock(&file->lock);

    if (access(file->filename, F_OK) < 0) {
        /* create KV file when not exist */
        if (create_hash_file(file) < 0) {
            goto fail;
        }
    }
    // 文件存在情况下对文件大小检查 如果不对打印log
    // 特殊情况可能大小不对 如创建文件过程被中断
    // get file size 
    
    stat(file->filename, &st);
    size = st.st_size;
    //printf("**  %d \n", size);
    if(size < ITEM_MAX_LEN * TABLE_ROW_SIZE * TABLE_COL_SIZE)
        kv_err("file size less than expect\n");
    
    pthread_mutex_unlock(&file->lock);
    return file;
fail:
    pthread_mutex_unlock(&file->lock);
    free(file);

    return NULL;
}

static int __kv_get(kv_file_t *file, const char *key, void *value, int *value_len)
{
    int ret;
    if (!file || !key || !value || !value_len || *value_len <= 0) {
        return MAPFILE_FAIL;
    }

    pthread_mutex_lock(&file->lock);
    ret = hash_table_get(file, key, value, value_len);
    pthread_mutex_unlock(&file->lock);

    return ret;
}

static int __kv_set(kv_file_t *file, const char *key, void *value, int value_len)
{
    int ret;
    if (!file || !key || !value || value_len <= 0) {
        return MAPFILE_FAIL;
    }

    pthread_mutex_lock(&file->lock);
    ret = hash_table_put(file, key, value, value_len);
    pthread_mutex_unlock(&file->lock);

    return ret;
}

static int __kv_del(kv_file_t *file, const  char *key)
{
    int ret;
    if (!file || !key) {
        return MAPFILE_FAIL;
    }

    /* remove old value if exist */
    pthread_mutex_lock(&file->lock);
    ret = hash_table_rm(file, key);
    pthread_mutex_unlock(&file->lock);

    return ret;
}

//static kv_file_t *file = NULL;
#if 0
static int kv_get(kv_file_t *file, const char *key, void *value, int *value_len)
{

    return __kv_get(file, key, value, value_len);
}

static int kv_set(kv_file_t *file, const char *key, void *value, int value_len)
{

    return __kv_set(file, key, value, value_len);
}

static int kv_del(kv_file_t *file, const char *key)
{
    #if 0
    if (!file) {
        file = kv_open(KV_FILE_NAME);
        if (!file) {
            kv_err("kv_open failed");
            return -1;
        }
    }
    #endif
    
    return __kv_del(file, key);
}
#endif


int mapfile_kv_set(kv_file_t *file, const char *key, const void *val, int value_len)
{
    return __kv_set(file, key, (void *)val, value_len);
}

int mapfile_kv_get(kv_file_t *file, const char *key, void *val, int *value_len)
{
    return __kv_get(file, key, val, value_len);
}

int mapfile_kv_del(kv_file_t *file, const char *key)
{

    return __kv_del(file, key);
}

int mapfile_kv_emp(kv_file_t *hash_kv)
{
    int fd = 0;
    FILE * fp = NULL;
    int ret = MAPFILE_FAIL;
    
    if (hash_kv == NULL) {
        return MAPFILE_FAIL;
    }
    pthread_mutex_lock(&hash_kv->lock);
    fp = fopen(hash_kv->filename, "w"); //用于清空该文件的内容
    if (fp == NULL)
        goto EXIT;
    
    ret = truncate(hash_kv->filename, ITEM_MAX_LEN * TABLE_ROW_SIZE * TABLE_COL_SIZE);//重新固化文件大小
    if(MAPFILE_FAIL == ret)
    {
        kv_err("mapfile re_truncate failed");
        goto EXIT;
    }

    fd = fileno(fp);
    if(fsync(fd) < 0)
    {
        ret = MAPFILE_FAIL;
        goto EXIT;
    }

    ret = MAPFILE_OK;
EXIT:
    if(NULL != fp)
        fclose(fp);
    
    pthread_mutex_unlock(&hash_kv->lock);
    return ret;

}



