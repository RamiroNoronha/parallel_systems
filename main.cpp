#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <iostream>
#include "passa_tempo.h"

#define log false
using namespace std;

struct PositionData
{
    int x;
    int y;
    int decimos;
};

struct thread_data
{
    int tid;
    int gid;
    int npos;
    int current_pos;
    PositionData *pos;
};

struct position
{
    int gid;
    int qtd;
};

// mutex and condition variable to control the start of the threads in the board
pthread_mutex_t startMutex;
pthread_cond_t startCond;
// variable that will be used to control the number of threads that are not started yet
int not_started_threads;

// mutex and condition variable to control the moviment of the threads in the board
pthread_mutex_t movimentMutex;
pthread_cond_t movimentCond;
// The board that will be used for the threads moviment
position **tabuleiro;

// function created for debugging purposes
void print_data(thread_data *data, int n_threads);

// function to centralize the initialization of the mutexes and condition variables
void init();

// function to destroy the mutexes and condition variables
void destroy();

// function responsible for start the moviment, it will be put all the threads in the first position
void *start(thread_data *arg);

// function responsible for free the old position. It's kind of the implemantion of the sair function recommended for the professor
void free_old_position(PositionData &oldPosition);

// function that crate the matrix of positions
void initialize_board(int N);

// function responsible for getting a new position for the thread. It's kind of the implemantion of the entrar function recommended for the professor
void get_new_position(PositionData &newPosition, thread_data *arg);

// function that verify if the thread finished the moviment
bool verify_finished_all_moviments(thread_data *arg);

// function responsible for the moviment of the threads. It will call the function that make the threads moviment in the board
void *moviment(thread_data *arg);

// function that will be called for each thread in its creation
void *routine(void *arg);

// function that will load the data of the threads. It receives the number of threads and the data of each thread
void load_threads_data(int n_threads, thread_data *threads, int N);

int main()
{
    int N, n_threads;

    cin >> N >> n_threads;
    not_started_threads = n_threads;
    initialize_board(N);

    thread_data *threads = new thread_data[n_threads];

    load_threads_data(n_threads, threads, N);

    if (log)
        print_data(threads, n_threads);

    pthread_t *threads_id = new pthread_t[n_threads];

    init();

    for (int i = 0; i < n_threads; i++)
    {
        pthread_create(&threads_id[i], NULL, routine, (void *)&threads[i]);
    }

    for (int i = 0; i < n_threads; i++)
    {
        pthread_join(threads_id[i], NULL);
    }

    for (int i = 0; i < n_threads; i++)
    {
        delete[] threads[i].pos;
    }
    delete[] threads;
    delete[] threads_id;
    destroy();
}

void *routine(void *arg)
{
    // First of all, we need to cast the arg to thread_data, that contains all information about the threads and its moviment
    thread_data *data = (thread_data *)arg;

    // Before we start the moviment, we need to call the function that will put all the threads in the first position, and wait until all threads are started
    start(data);

    // After all threads are started, we need to call the function that will make the threads moviment in the board
    moviment(data);

    return NULL;
}

void free_old_position(PositionData &oldPosition)
{
    // Note that we do not call mutex lock here, because we are already in the mutex lock in the moviment function

    // We descrese the quantity of threads in the old position, and if it is 0, we need to free the position for threads thar belongs to other groups
    tabuleiro[oldPosition.x][oldPosition.y].qtd--;
    if (tabuleiro[oldPosition.x][oldPosition.y].qtd == 0)
    {
        tabuleiro[oldPosition.x][oldPosition.y].gid = -1;
    }

    // We need to unlock the mutex to let other threads access the board
    pthread_mutex_unlock(&movimentMutex);
    // We need to signal the condition variable to wake up other threads that are waiting for the board
    pthread_cond_signal(&movimentCond);
}

void get_new_position(PositionData &newPosition, thread_data *arg)
{
    // Note that we do not call mutex lock here, because we are already in the mutex lock in the moviment function

    // We need to check if the new position is valid, and if it is not, we need to wait until it is valid
    while (tabuleiro[newPosition.x][newPosition.y].gid != arg->gid && tabuleiro[newPosition.x][newPosition.y].qtd > 0)
    {
        // We need to wait for the condition variable to wake up other threads that are waiting for the board
        pthread_cond_wait(&movimentCond, &movimentMutex);
    }

    // We need to indicate that this position is occupied for the gorup and the thread
    tabuleiro[newPosition.x][newPosition.y].gid = arg->gid;
    tabuleiro[newPosition.x][newPosition.y].qtd++;
}

bool verify_finished_all_moviments(thread_data *arg)
{
    // We need to check if the thread finished all the moviments, and if it is true, we need to free the old position and break the loop (get out of the board)
    if (arg->current_pos >= arg->npos)
        return true;

    return false;
}

void *moviment(thread_data *arg)
{
    PositionData oldPosition;
    PositionData newPosition;

    // We will stay in moviment until the thread finish its moviment
    while (true)
    {
        // Before we start the moviment, we need to lock the mutex to prevent other threads to access the board
        pthread_mutex_lock(&movimentMutex);
        // We get the old position of the thread to free it after we get the new position
        oldPosition = arg->pos[arg->current_pos];
        arg->current_pos++;

        // This part verify if the thread finished all the moviments, and if it is true, we need to free the old position and break the loop (get out of the board)
        if (verify_finished_all_moviments(arg))
        {
            free_old_position(oldPosition);
            break;
        }

        // We need to get the new position of the thread, and we need to lock the mutex to prevent other threads to access the board
        newPosition = arg->pos[arg->current_pos];

        // This function tries to get a new position for the thread, and if it is not possible, it will wait until it is possible
        get_new_position(newPosition, arg);
        // This function will free the old position of the thread, and it will signal the condition variable to wake up other threads that are waiting for the board
        free_old_position(oldPosition);
        // The thread will sleep for the time that it takes to move to the new position
        passa_tempo(arg->tid, newPosition.x, newPosition.y, newPosition.decimos);
    }
    return NULL;
}

void *start(thread_data *arg)
{
    if (log)
    {
        printf("Thread %d started\n", arg->tid);
    }
    PositionData positionData = arg->pos[arg->current_pos];

    // We lock the mutex to prevent just one thread to access the board for time
    pthread_mutex_lock(&startMutex);

    // We decrese the number of threads that are not started yet
    not_started_threads--;

    // We put the thread in the first position
    tabuleiro[positionData.x][positionData.y].gid = arg->gid;
    tabuleiro[positionData.x][positionData.y].qtd++;

    // If it is the last thread to start, we need to wake up all the threads that are waiting for the start
    if (not_started_threads == 0)
    {
        pthread_cond_broadcast(&startCond);
    }

    // If there are still threads that are not started yet, we need to wait for them
    while (not_started_threads > 0)
    {
        // We need to unlock the mutex to let other threads access the board until there are threads that are not started yet
        pthread_cond_wait(&startCond, &startMutex);
    }

    // We free the mutex to let other threads access the board
    pthread_mutex_unlock(&startMutex);
    // We call the passa_tempo function to simulate the time that the thread will take to in the position
    passa_tempo(arg->tid, positionData.x, positionData.y, positionData.decimos);
    if (log)
    {
        printf("Thread %d finished start\n", arg->tid);
    }
    return NULL;
}

void load_threads_data(int n_threads, thread_data *threads, int N)
{
    int tid, gid, npos;
    int x, y, decimos;
    for (int i = 0; i < n_threads; i++)
    {
        cin >> tid >> gid >> npos;
        threads[i].tid = tid;
        threads[i].gid = gid;
        threads[i].npos = npos;
        threads[i].current_pos = 0;
        threads[i].pos = new PositionData[threads[i].npos];
        for (int j = 0; j < N; j++)
        {
            cin >> x >> y >> decimos;
            threads[i].pos[j] = {x, y, decimos};
        }
    }
}

void initialize_board(int N)
{
    tabuleiro = new position *[N];
    for (int i = 0; i < N; i++)
    {
        tabuleiro[i] = new position[N];

        for (int j = 0; j < N; j++)
        {
            tabuleiro[i][j] = {-1, 0};
        }
    }
}

void destroy()
{
    pthread_mutex_destroy(&startMutex);
    pthread_cond_destroy(&startCond);
    pthread_mutex_destroy(&movimentMutex);
    pthread_cond_destroy(&movimentCond);
}

void init()
{
    pthread_mutex_init(&startMutex, NULL);
    pthread_cond_init(&startCond, NULL);
    pthread_mutex_init(&movimentMutex, NULL);
    pthread_cond_init(&movimentCond, NULL);
}

void print_data(thread_data *data, int n_threads)
{
    for (int i = 0; i < n_threads; i++)
    {
        cout << "Position " << i << ": " << endl;
        cout << "Thread ID: " << data[i].tid << ", Group ID: " << data[i].gid << ", Number of Positions: " << data[i].npos << endl;
        for (int j = 0; j < data[i].npos; j++)
        {
            cout << "Position: (" << data[i].pos[j].x << ", " << data[i].pos[j].y << "), Decimos: " << data[i].pos[j].decimos << endl;
        }
    }
}
