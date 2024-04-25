#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <libxml/HTMLparser.h>
#include <curl/curl.h>

#define LOG_FILE "crawler.log" // Log file path
#define MAX_DEPTH 3 // Maximum depth for crawling

// Define a structure for queue elements.
typedef struct URLQueueNode {
    char *url;
    int depth;
    struct URLQueueNode *next;
} URLQueueNode;

// Define a structure for a thread-safe queue.
typedef struct {
    URLQueueNode *head, *tail;
    pthread_mutex_t lock;
} URLQueue;

FILE *log_file;

// Initialize a URL queue.
void initQueue(URLQueue *queue) {
    queue->head = queue->tail = NULL;
    pthread_mutex_init(&queue->lock, NULL);
}

// Add a URL to the queue.
void enqueue(URLQueue *queue, const char *url, int depth) {
    URLQueueNode *newNode = malloc(sizeof(URLQueueNode));
    if (newNode == NULL) {
        fprintf(log_file, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->url = strdup(url);
    if (newNode->url == NULL) {
        fprintf(log_file, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->depth = depth;
    newNode->next = NULL;

    pthread_mutex_lock(&queue->lock);
    // If queue is not empty, append the new node to the end, else set both head and tail to the new node
    if (queue->tail) {
        queue->tail->next = newNode;
    } else {
        queue->head = newNode;
    }
    queue->tail = newNode;
    pthread_mutex_unlock(&queue->lock);
}

// Remove a URL from the queue.
URLQueueNode *dequeue(URLQueue *queue) {
    pthread_mutex_lock(&queue->lock);
    if (queue->head == NULL) {
        pthread_mutex_unlock(&queue->lock);
        return NULL;
    }

    URLQueueNode *temp = queue->head;
    queue->head = queue->head->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    pthread_mutex_unlock(&queue->lock);
    return temp;
}

// Frees memory allocated for the queue
void destroyQueue(URLQueue *queue) {
    URLQueueNode *current = queue->head;
    while (current != NULL) {
        URLQueueNode *next = current->next;
        free(current->url);
        free(current);
        current = next;
    }
    pthread_mutex_destroy(&queue->lock);
}

// Callback function to handle writing HTTP response data
size_t write_callback(char *ptr, size_t size, size_t nmemb, char **html_content) {
    size_t total_size = size * nmemb;
    *html_content = realloc(*html_content, total_size + 1);
    if (*html_content == NULL) {
        fprintf(log_file, "Memory allocation failed\n");
        return 0;
    }
    memcpy(*html_content, ptr, total_size);
    (*html_content)[total_size] = '\0';
    return total_size;
}

// Function to fetch HTML content from a URL
char *fetch_html_content(const char *url) {
    CURL *curl;
    CURLcode res;
    char *html_content = NULL;

    curl = curl_easy_init(); // Initialize libcurl
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url); // Set URL to fetch
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_callback); // Set callback for writing data
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html_content); // Set pointer to write data to
        res = curl_easy_perform(curl); // Perform the request
        //fprintf(log_file, "curl_easy_perform() returned: %d\n", res);
        if (res != CURLE_OK) {
            fprintf(log_file, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            curl_easy_cleanup(curl);
            return NULL;
        }
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        //fprintf(log_file, "HTTP response code: %ld\n", response_code);
        if (response_code != 200) {
            fprintf(log_file, "Request failed with HTTP status code %ld\n", response_code);
            curl_easy_cleanup(curl);
            return NULL;
        }
        curl_easy_cleanup(curl);
    } else {
        fprintf(log_file, "Failed to initialize curl\n");
        return NULL;
    }

    return html_content;
}

// Function to parse HTML content and enqueue URLs found within it
void parse_html(const char *html_content, URLQueue *queue, int depth) {
    fprintf(log_file, "Parsing HTML content for depth %d\n", depth);

    const char *cursor = html_content;
    while ((cursor = strstr(cursor, "<a")) != NULL) {
        const char *href_start = strstr(cursor, "href=\"");
        if (href_start != NULL) {
            href_start += 6; // Move past "href=\""
            const char *href_end = strchr(href_start, '"');
            if (href_end != NULL) {
                size_t url_length = href_end - href_start;
                char *url = (char *)malloc(url_length + 1);
                if (url != NULL) {
                    strncpy(url, href_start, url_length);
                    url[url_length] = '\0';
                    enqueue(queue, url, depth + 1);
                }
                cursor = href_end;
            }
        }
        cursor++;
    }
}

// Placeholder for the function to fetch and process a URL
void *fetch_url(void *arg) {
    URLQueue *queue = (URLQueue *)arg;

    URLQueueNode *node;
    while ((node = dequeue(queue)) != NULL) {
        if (node->depth >= MAX_DEPTH) {
            free(node->url);
            free(node);
            continue;
        }

        fprintf(log_file, "Processing URL: %s\n", node->url);

        char *html_content = fetch_html_content(node->url);
        if (html_content != NULL) {
            parse_html(html_content, queue, node->depth);
            free(html_content);
        }

        fprintf(log_file, "Processed URL: %s\n", node->url);
        free(node->url);
        free(node);
    }

    return NULL;
}

// Main function to drive the web crawler.
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <starting-url> [additional-urls...]\n", argv[0]);
        return 1;
    }

    log_file = fopen(LOG_FILE, "w");
    if (log_file == NULL) {
        fprintf(stderr, "Error: Failed to open log file for writing\n");
        return 1;
    }

    URLQueue queue;
    initQueue(&queue);

    for (int i = 1; i < argc; i++) {
        //fprintf(log_file, "Enqueuing URL: %s\n", argv[i]);
        enqueue(&queue, argv[i], 0);
    }

    URLQueueNode *current = queue.head;
    while (current != NULL) {
        //fprintf(log_file, "%s\n", current->url);
        current = current->next;
    }

    const int NUM_THREADS = 4;
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, fetch_url, (void *)&queue);
    }
    
    // Join threads after completion
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    destroyQueue(&queue);

    fclose(log_file);

    return 0;
}
