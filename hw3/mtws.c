#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_FILENAME_LENGTH 256 // 파일 이름의 최대 길이

// 전역 변수
char **buffer; // 버퍼를 위한 동적 배열
int buffer_size = 10; // 기본 버퍼 크기, 명령줄을 통해 설정 가능
int count = 0; // 버퍼 내 항목 수
int in = 0; // 버퍼에 다음으로 삽입할 위치
int out = 0; // 버퍼에서 다음으로 제거할 위치

// 동기화 프리미티브
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_nonempty = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_nonfull = PTHREAD_COND_INITIALIZER;

int total_found = 0; // 찾은 단어의 총 개수
int producer_done = 0; // 생산자가 완료되었음을 나타내는 플래그
int num_files = 0; // 단어를 포함하는 처리된 파일 수
int global_file_counter = 0; // 고유 파일 번호를 위한 전역 카운터

// 함수 프로토타입
void* producer(void* arg);
void* consumer(void* arg);
int search_in_file(char* filename, char* word);
void print_usage();

// strcasestr 함수 정의
char* strcasestr(const char* haystack, const char* needle) {
    if (!*needle)
        return (char*)haystack;
    for (const char* p1 = haystack; *p1; p1++) {
        const char* p1Begin = p1;
        const char* p2 = needle;
        while (*p1 && *p2 && (tolower((unsigned char)*p1) == tolower((unsigned char)*p2))) {
            p1++;
            p2++;
        }
        if (!*p2)
            return (char*)p1Begin;
        p1 = p1Begin; // restore p1
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    int num_threads = 3; // 기본 소비자 스레드 수
    char* directory = NULL;
    char* search_word = NULL;

    // 명령줄 인수 파싱
    int opt;
    while ((opt = getopt(argc, argv, "b:t:d:w:")) != -1) {
        switch (opt) {
            case 'b':
                buffer_size = atoi(optarg);
                break;
            case 't':
                num_threads = atoi(optarg);
                break;
            case 'd':
                directory = optarg;
                break;
            case 'w':
                search_word = optarg;
                break;
            default:
                print_usage();
                exit(EXIT_FAILURE);
        }
    }

    // 디버깅을 위한 파싱된 인수 출력
    printf("Buffer size=%d, Num threads=%d, Directory=%s, Searchword=%s\n", buffer_size, num_threads, directory, search_word);

    // 버퍼를 위한 메모리 할당
    buffer = (char**)malloc(buffer_size * sizeof(char*));
    for(int i = 0; i < buffer_size; i++) {
        buffer[i] = (char*)malloc(MAX_FILENAME_LENGTH);
    }

    // 필요한 인수가 제공되었는지 확인
    if (directory == NULL || search_word == NULL) {
        print_usage();
        exit(EXIT_FAILURE);
    }

    // 생산자 및 소비자 스레드 생성
    pthread_t prod, cons[num_threads];
    pthread_create(&prod, NULL, producer, (void*)directory);
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&cons[i], NULL, consumer, (void*)search_word);
        printf("[Thread %lu] started searching '%s' ...\n", (unsigned long)cons[i], search_word);

    }

    // 스레드가 완료될 때까지 대기
    pthread_join(prod, NULL);
    for (int i = 0; i < num_threads; i++) {
        pthread_join(cons[i], NULL);
    }

    // 최종 결과 출력
    printf("Total found = %d (Num files = %d)\n", total_found, num_files);

    // 할당된 메모리 해제
    for(int i = 0; i < buffer_size; i++) {
        free(buffer[i]);
    }
    free(buffer);

    return 0;
}

// 생산자 스레드 함수
void* producer(void* arg) {
    char* directory = (char*)arg;
    DIR* dirp;
    struct dirent* entry;
    dirp = opendir(directory);

    if (dirp == NULL) {
        printf("Directory cannot be opened.\n");
        return NULL;
    }

    // 디렉터리 항목 읽기
    while ((entry = readdir(dirp)) != NULL) {
        pthread_mutex_lock(&mutex);
        while (count == buffer_size) {
            // 버퍼가 가득 차면 대기
            pthread_cond_wait(&cond_nonfull, &mutex);
        }

        // 정규 파일을 버퍼에 추가
        if (entry->d_type == DT_REG) {
            int ret = snprintf(buffer[in], MAX_FILENAME_LENGTH, "%s/%s", directory, entry->d_name);
            if (ret >= MAX_FILENAME_LENGTH) {
                fprintf(stderr, "Filename truncated: %s/%s\n", directory, entry->d_name);
            }
            in = (in + 1) % buffer_size;
            count++;
            pthread_cond_signal(&cond_nonempty); // 소비자에게 신호
        }

        pthread_mutex_unlock(&mutex);
    }

    // 생산자가 완료되었음을 표시하고 모든 소비자에게 신호
    producer_done = 1;
    pthread_cond_broadcast(&cond_nonempty);

    closedir(dirp);
    return NULL;
}

// 소비자 스레드 함수
void* consumer(void* arg) {
    char* word = (char*)arg;
    char filename[MAX_FILENAME_LENGTH];
    pthread_t thread_id = pthread_self();

    while (1) {
        pthread_mutex_lock(&mutex);

        while (count == 0) {
            if (producer_done) {
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
            // 버퍼가 비어 있으면 대기
            pthread_cond_wait(&cond_nonempty, &mutex);
        }

        // 버퍼에서 파일 제거
        strcpy(filename, buffer[out]);
        out = (out + 1) % buffer_size;
        count--;
        pthread_cond_signal(&cond_nonfull); // 생산자에게 신호
        pthread_mutex_unlock(&mutex);

        int thread_number;
        pthread_mutex_lock(&mutex);
        thread_number = global_file_counter++;
        pthread_mutex_unlock(&mutex);

        // 파일에서 단어 검색
        int found = search_in_file(filename, word);
        if (found > 0) {
            pthread_mutex_lock(&mutex);
            total_found += found;
            num_files++;
            pthread_mutex_unlock(&mutex);
        }

        // 스레드가 처리한 각 파일에 대한 결과 출력
        printf("[Thread %lu-%d] %s : %d found\n", (unsigned long)thread_id, thread_number, filename, found);
    }

    return NULL;
}

// 파일에서 단어 검색 함수
int search_in_file(char* filename, char* word) {
    FILE* file;
    char line[256];
    int found = 0;

    file = fopen(filename, "r");
    if (file == NULL) {
        printf("Cannot open file %s\n", filename);
        return 0;
    }

    // 각 줄을 읽고 단어를 검색
    while (fgets(line, sizeof(line), file)) {
        if (strcasestr(line, word) != NULL) {
            found++;
        }
    }

    fclose(file);
    return found;
}

// 사용법 출력 함수
void print_usage() {
    printf("Usage: ./mtws\n");
    printf("-b : bounded buffer size\n");
    printf("-t : number of threads searching word (except for main thread)\n");
    printf("-d : search directory\n");
    printf("-w : search word\n");
}
