#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <iostream>

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

pthread_mutex_t startMutex;
pthread_cond_t startCond;
int not_started_threads;
pthread_mutex_t movimentMutex;
pthread_cond_t movimentCond;
position **tabuleiro;

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

void passa_tempo(int tid, int x, int y, int decimos)
{
    struct timespec zzz, agora;
    static struct timespec inicio = {0, 0};
    int tstamp;

    if ((inicio.tv_sec == 0) && (inicio.tv_nsec == 0))
    {
        clock_gettime(CLOCK_REALTIME, &inicio);
    }

    zzz.tv_sec = decimos / 10;
    zzz.tv_nsec = (decimos % 10) * 100L * 1000000L;

    clock_gettime(CLOCK_REALTIME, &agora);
    tstamp = (10 * agora.tv_sec + agora.tv_nsec / 100000000L) - (10 * inicio.tv_sec + inicio.tv_nsec / 100000000L);

    printf("%3d [ %2d @(%2d,%2d) z%4d\n", tstamp, tid, x, y, decimos);

    nanosleep(&zzz, NULL);

    clock_gettime(CLOCK_REALTIME, &agora);
    tstamp = (10 * agora.tv_sec + agora.tv_nsec / 100000000L) - (10 * inicio.tv_sec + inicio.tv_nsec / 100000000L);

    printf("%3d ) %2d @(%2d,%2d)\n", tstamp, tid, x, y);
}

void init()
{
    pthread_mutex_init(&startMutex, NULL);
    pthread_cond_init(&startCond, NULL);
    pthread_mutex_init(&movimentMutex, NULL);
    pthread_cond_init(&movimentCond, NULL);
}

void destroy()
{
    pthread_mutex_destroy(&startMutex);
    pthread_cond_destroy(&startCond);
    pthread_mutex_destroy(&movimentMutex);
    pthread_cond_destroy(&movimentCond);
}

void *start(thread_data *arg)
{
    if (log)
    {
        printf("Thread %d started\n", arg->tid);
    }
    PositionData positionData = arg->pos[arg->current_pos];

    pthread_mutex_lock(&startMutex);
    not_started_threads--;
    tabuleiro[positionData.x][positionData.y].gid = arg->gid;
    tabuleiro[positionData.x][positionData.y].qtd++;

    if (not_started_threads == 0)
    {
        pthread_cond_broadcast(&startCond);
    }
    while (not_started_threads > 0)
    {
        pthread_cond_wait(&startCond, &startMutex);
    }
    pthread_mutex_unlock(&startMutex);
    passa_tempo(arg->tid, positionData.x, positionData.y, positionData.decimos);
    if (log)
    {
        printf("Thread %d finished start\n", arg->tid);
    }
    return NULL;
}

void free_old_position(PositionData &oldPosition);

void initialize_board(int N);

void get_new_position(PositionData &newPosition, thread_data *arg);

bool verify_finished_movement(thread_data *arg);

void *moviment(thread_data *arg)
{
    PositionData oldPosition;
    PositionData newPosition;

    while (true)
    {
        pthread_mutex_lock(&movimentMutex);
        oldPosition = arg->pos[arg->current_pos];
        arg->current_pos++;

        if (verify_finished_movement(arg))
        {
            free_old_position(oldPosition);
            break;
        }

        newPosition = arg->pos[arg->current_pos];

        get_new_position(newPosition, arg);
        free_old_position(oldPosition);
        passa_tempo(arg->tid, newPosition.x, newPosition.y, newPosition.decimos);
    }
    return NULL;
}

bool verify_finished_movement(thread_data *arg)
{

    if (arg->current_pos >= arg->npos)
        return true;

    return false;
}

void get_new_position(PositionData &newPosition, thread_data *arg)
{
    while (tabuleiro[newPosition.x][newPosition.y].gid != arg->gid && tabuleiro[newPosition.x][newPosition.y].qtd > 0)
    {
        pthread_cond_wait(&movimentCond, &movimentMutex);
    }

    tabuleiro[newPosition.x][newPosition.y].gid = arg->gid;
    tabuleiro[newPosition.x][newPosition.y].qtd++;
}

void free_old_position(PositionData &oldPosition)
{
    tabuleiro[oldPosition.x][oldPosition.y].qtd--;
    if (tabuleiro[oldPosition.x][oldPosition.y].qtd == 0)
    {
        tabuleiro[oldPosition.x][oldPosition.y].gid = -1;
    }
    pthread_mutex_unlock(&movimentMutex);
    pthread_cond_signal(&movimentCond);
}

void *routine(void *arg)
{
    thread_data *data = (thread_data *)arg;
    start(data);

    moviment(data);

    return NULL;
}

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
