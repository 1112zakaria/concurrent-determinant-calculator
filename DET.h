#pragma once

struct shared_use_st {
    int determinant;
    int largest;
    //idk
};

int compute_determinant(int[3][3]);
void create_processes();
void create_shared_memory(int*);
void attach_shared_memory(struct shared_use_st**,int);
void detach_shared_memory(struct shared_use_st*);
void delete_shared_memory(int);
void handle_signal(int);
void add_to_shared_memory(int*,int);
static int set_semvalue(void);
static void del_semvalue(void);
static int semaphore_p(void);
static int semaphore_v(void);



