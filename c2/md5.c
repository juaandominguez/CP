#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <openssl/evp.h>
#include <threads.h>
#include "options.h"
#include "queue.h"


#define MAX_PATH 1024
#define BLOCK_SIZE (10*1024*1024)
#define MAX_LINE_LENGTH (MAX_PATH * 2)


struct file_md5 {
    char *file;
    unsigned char *hash;
    unsigned int hash_size;
};

struct args{
    char *dir;
    queue q;
};  

struct args2{
    queue in_q;
    queue out_q;
    struct options opt;
};
struct args3{
    queue out_q;
    struct options opt;
};

struct readArgs{
    char *file;
    char *dir;
    queue q;
};

int get_entries(void *args);


void print_hash(struct file_md5 *md5) {
    for(int i = 0; i < md5->hash_size; i++) {
        printf("%02hhx", md5->hash[i]);
    }
}


int read_hash_file(void *args) {
    struct readArgs *readArgs = args;
    FILE *fp;
    char line[MAX_LINE_LENGTH];
    char *file_name, *hash;
    int hash_len;

    if((fp = fopen(readArgs->file, "r")) == NULL) {
        printf("Could not open %s : %s\n", readArgs->file, strerror(errno));
        exit(0);
    }

    while(fgets(line, MAX_LINE_LENGTH, fp) != NULL) {
        struct file_md5 *md5 = malloc(sizeof(struct file_md5));
        file_name = strtok(line, ": ");
        hash      = strtok(NULL, ": ");
        hash_len  = strlen(hash);

        md5->file      = malloc(strlen(file_name) + strlen(readArgs->dir) + 2);
        sprintf(md5->file, "%s/%s", readArgs->dir, file_name);
        md5->hash      = malloc(hash_len / 2);
        md5->hash_size = hash_len / 2;


        for(int i = 0; i < hash_len; i+=2)
            sscanf(hash + i, "%02hhx", &md5->hash[i / 2]);

        q_insert(readArgs->q, md5);
    }
    q_finish(readArgs->q);
    fclose(fp);
    return 0;
}


void sum_file(struct file_md5 *md5) {
    EVP_MD_CTX *mdctx;
    int nbytes;
    FILE *fp;
    char *buf;

    if((fp = fopen(md5->file, "r")) == NULL) {
        printf("Could not open %s\n", md5->file);
        return;
    }

    buf = malloc(BLOCK_SIZE);
    const EVP_MD *md = EVP_get_digestbyname("md5");

    mdctx = EVP_MD_CTX_create();
    EVP_DigestInit_ex(mdctx, md, NULL);

    while((nbytes = fread(buf, 1, BLOCK_SIZE, fp)) >0)
        EVP_DigestUpdate(mdctx, buf, nbytes);

    md5->hash = malloc(EVP_MAX_MD_SIZE);
    EVP_DigestFinal_ex(mdctx, md5->hash, &md5->hash_size);

    EVP_MD_CTX_destroy(mdctx);
    free(buf);
    fclose(fp);
}


void recurse(char *entry, void *arg) {
    queue q = * (queue *) arg;
    struct stat st;
    struct args *args = malloc(sizeof(struct args));
    args->dir = entry;
    args->q = q;
    stat(entry, &st);

    if(S_ISDIR(st.st_mode)) get_entries(args);
        free(args); 
}


void add_files(char *entry, void *arg) {
    queue q = * (queue *) arg;
    struct stat st;

    stat(entry, &st);

    if(S_ISREG(st.st_mode))
        q_insert(q, strdup(entry));
}


void walk_dir(char *dir, void (*action)(char *entry, void *arg), void *arg) {
    DIR *d;
    struct dirent *ent;
    char full_path[MAX_PATH];

    if((d = opendir(dir)) == NULL) {
        printf("Could not open dir %s\n", dir);
        return;
    }

    while((ent = readdir(d)) != NULL) {
        if(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") ==0)
            continue;

        snprintf(full_path, MAX_PATH, "%s/%s", dir, ent->d_name);

        action(full_path, arg);
    }

    closedir(d);
}


int get_entries(void *args2) {
    struct args *args = args2;
    char *dir =args->dir;
    queue q = args->q;
    walk_dir(dir, add_files, &q);
    walk_dir(dir, recurse, &q);
    return 0;
}

int entries(void *args2){
    struct args *args = args2;
    get_entries (args2);
    q_finish(args->q);
    return 0;
}

int reading(void * args){
    struct readArgs *readArgs = args;
    struct file_md5 *md5_in, md5_file;
    while((md5_in = q_remove(readArgs->q))) {
        md5_file.file = md5_in->file;

        sum_file(&md5_file);

        if(memcmp(md5_file.hash, md5_in->hash, md5_file.hash_size)!=0) {
            printf("File %s doesn't match.\nFound:    ", md5_file.file);
            print_hash(&md5_file);
            printf("\nExpected: ");
            print_hash(md5_in);
            printf("\n");
        }

        free(md5_file.hash);

        free(md5_in->file);
        free(md5_in->hash);
        free(md5_in);
    }
    return 0;
}


void check(struct options opt) {
    queue in_q;
    
    thrd_t * thread = malloc(sizeof(thrd_t));
    struct readArgs *readArgs = malloc(sizeof(struct readArgs));
    thrd_t * thread2 = malloc(sizeof(thrd_t)*opt.num_threads);
    struct readArgs *readArgs2 = malloc(sizeof(struct readArgs)*opt.num_threads);
    in_q  = q_create(opt.queue_size);
    readArgs->file = opt.file;
    readArgs->dir = opt.dir;
    readArgs->q = in_q;
    thrd_create(thread, read_hash_file, readArgs);
    for(int i = 0; i < opt.num_threads; i++){
        readArgs2[i].q = in_q;
        thrd_create(&thread2[i], reading, &readArgs2[i]);
    }
    for(int i = 0; i < opt.num_threads; i++){
        thrd_join(thread2[i], NULL);
    }
    q_destroy(in_q);
    free(thread);
    free(readArgs);
    free(thread2);
    free(readArgs2);
}

int hashing(void *args2){
    struct file_md5 *md5;
    struct args2 *args = args2;
    char *ent;
    queue in_q = args->in_q;
    queue out_q = args->out_q;
    while((ent = q_remove(in_q)) != NULL) {
        md5 = malloc(sizeof(struct file_md5));

        md5->file = ent;
        sum_file(md5);

        q_insert(out_q, md5);
    }
    return 0;
}

int writing(void *args3){
    struct args3 *args = args3;
    struct file_md5 *md5;
    FILE *out;
    int dirname_len;
    if((out = fopen(args->opt.file, "w")) == NULL) {
        printf("Could not open output file\n");
        exit(0);
    }
    dirname_len = strlen(args->opt.dir) + 1; // length of dir + /

    while((md5 = q_remove(args->out_q)) != NULL) {
        fprintf(out, "%s: ", md5->file + dirname_len);

        for(int i = 0; i < md5->hash_size; i++)
            fprintf(out, "%02hhx", md5->hash[i]);
        fprintf(out, "\n");

        free(md5->file);
        free(md5->hash);
        free(md5);
    }
    fclose(out);
    return 0;
}


void sum(struct options opt) {
    queue in_q, out_q;
    thrd_t * entriesThread = malloc(sizeof(thrd_t));
    thrd_t * writeThread = malloc(sizeof(thrd_t));
    struct args *args = malloc(sizeof(struct args));
    in_q  = q_create(opt.queue_size);
    out_q = q_create(opt.queue_size);
    args->q = in_q;
    args->dir = opt.dir;

    thrd_create(entriesThread, entries, args);

    thrd_t * thread2 = malloc(sizeof(thrd_t)*opt.num_threads);
    struct args2 *args2 = malloc(sizeof(struct args2)*opt.num_threads);
    for(int i = 0; i < opt.num_threads; i++){
        args2[i].in_q = in_q;
        args2[i].out_q = out_q;
        args2[i].opt = opt;
        thrd_create(&thread2[i], hashing, &args2[i]);
    }
    
    struct args3 *args3 = malloc(sizeof(struct args3));
    args3->out_q = out_q;
    args3->opt = opt;
    thrd_create(writeThread, writing, args3);
    for(int i = 0; i < opt.num_threads; i++){
        thrd_join(thread2[i], NULL);
    }
    thrd_join(*entriesThread, NULL);
    q_finish(out_q);
    thrd_join(*writeThread, NULL);
    q_destroy(in_q);
    q_destroy(out_q);
    free(entriesThread);
    free(writeThread);
    free(args);
    free(thread2);
    free(args2);
    free(args3);
}


int main(int argc, char *argv[]) {

    struct options opt;

    opt.num_threads = 5;
    opt.queue_size  = 1000;
    opt.check       = true;
    opt.file        = NULL;
    opt.dir         = NULL;

    read_options (argc, argv, &opt);

    if(opt.check)
        check(opt);
    else
        sum(opt);
}
