#include <stdio.h>
#include <pthread.h>
#include <curl/curl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define MAX_URLS 10
#define MAX_URL_LENGTH 1000
// To keep track the total download overall
long total_downloaded = 0;
long total_file_size = 0;

// To kept the URL Track
char URLs[MAX_URLS][MAX_URL_LENGTH];
int url_count = 0; // to track how many URLs entered

// made structure so we pass the information of each chunk to thread function void *download_chunk(void *arg) which doesnot allow to pass all argument directly
typedef struct
{
    char URL[1000];
    long start;
    long end;
    int part_num;
} CHUNK;

//===============================================================================================================================================

// This function basically calculate the overall size of the file we want to download

long get_file_size(const char *URL)
{
    CURL *curl = curl_easy_init();

    curl_off_t file_size = 0;

    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, URL);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);      // Find the filesize skipping unnecessary content
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L); // Print the error if occurs

        // Get the size of the file need to be download so we divide them into chunks and assign them to number of threads
        if (curl_easy_perform(curl) == CURLE_OK)
        {
            curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &file_size);
        }
        else
        {
            file_size = -1; // Return -1 if an error occurs
        }
        curl_easy_cleanup(curl);
    }

    return (long)file_size;
}

//==========================================================================================================================================

// This write function tracks how much data each thread writes
// helps to calculate the current downloaded data in percentage
// Global mutex lock
pthread_mutex_t progress_lock = PTHREAD_MUTEX_INITIALIZER;

// This write function tracks how much data each thread writes
size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);

    // Critical Section: Update progress safely
    pthread_mutex_lock(&progress_lock);
    total_downloaded += written * size;
    pthread_mutex_unlock(&progress_lock);

    return written * size;
}

void *progress_display(void *arg)
{
    (void)arg;
    while (1)
    {
        long downloaded;

        // Critical Section: Read progress safely
        pthread_mutex_lock(&progress_lock);
        downloaded = total_downloaded;
        pthread_mutex_unlock(&progress_lock);

        double percent = (double)downloaded / total_file_size * 100.0;

        printf("\rDownload Progress: %.2f%%", percent);
        fflush(stdout);

        if (downloaded >= total_file_size)
            break;

        usleep(500000); // Refresh every 0.5 sec
    }
    printf("\n");
    return NULL;
}

// Here the multi threading part starts downloading data into chunks
void *download_chunks(void *arg)
{
    // type cast the argument into CHUNK struct to get the info like url etc
    CHUNK *chunk = (CHUNK *)arg;

    CURL *curl = curl_easy_init();

    if (!curl)
    {
        printf("Failed to initialize...\n");
        return NULL;
    }

    // range: "start-end" - tells curl which chunk (byte range) to download
    char range[100];
    snprintf(range, sizeof(range), "%ld-%ld", chunk->start, chunk->end);

    // filename: "part_0" etc. - tells where to save that chunk (each part gets its own file)
    char filename[20];
    snprintf(filename, sizeof(filename), "part_%d", chunk->part_num);

    FILE *fp = fopen(filename, "wb");
    if (!fp)
    {
        curl_easy_cleanup(curl);
        return NULL;
    }

    curl_easy_setopt(curl, CURLOPT_URL, chunk->URL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    // custom writer to track the bytes we have download so we get the percentage of the current download progress
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_RANGE, range);

    curl_easy_perform(curl);

    fclose(fp);
    curl_easy_cleanup(curl);
    return NULL;
}

void divide_file_into_chunks(const char *URL, int num_parts)
{
    long file_size = get_file_size(URL);

    if (file_size <= 0)
    {
        printf("Failed to get the filesize\n");
        return;
    }

    // create same number of threads and chunks and then assign each thread to each chunck
    CHUNK chunk[num_parts];
    pthread_t threads[num_parts];

    long chunk_size = file_size / num_parts;
    long start = 0;
    long end = 0;

    for (int i = 0; i < num_parts; i++)
    {
        start = i * chunk_size;
        if (i == num_parts - 1)
            end = file_size - 1; // Last part: go to the very end of the file
        else
            end = start + chunk_size - 1; // Other parts: go just until the end of this chunk

        chunk[i].start = start;
        chunk[i].end = end;
        chunk[i].part_num = i;

        // Copy the URL into the struct
        strncpy(chunk[i].URL, URL, sizeof(chunk[i].URL) - 1);
        chunk[i].URL[sizeof(chunk[i].URL) - 1] = '\0'; // Ensuring null-termination

        // Create a thread to download the corresponding chunk
        pthread_create(&threads[i], NULL, download_chunks, &chunk[i]);
    }

    // Wait for all threads to finish downloading
    for (int i = 0; i < num_parts; ++i)
    {
        pthread_join(threads[i], NULL);
    }
}

void merge_parts(const char *final_filename, int num_parts)
{
    FILE *final = fopen(final_filename, "wb");
    if (!final)
    {
        printf("Failed to create final file.\n");
        return;
    }

    for (int i = 0; i < num_parts; ++i)
    {
        char part_filename[20];
        snprintf(part_filename, sizeof(part_filename), "part_%d", i);

        FILE *part = fopen(part_filename, "rb");
        if (!part)
        {
            printf("Failed to open part %d\n", i);
            fclose(final);
            return;
        }

        char buffer[4096];
        size_t bytes;
        while ((bytes = fread(buffer, 1, sizeof(buffer), part)) > 0)
        {
            fwrite(buffer, 1, bytes, final);
        }

        fclose(part);
        remove(part_filename);
    }

    fclose(final);
}

//============================================================================================
void menu()
{
    printf("\n=================== MULTI THREADED DOWNLOAD MANAGER ===================\n");
    printf("\t\t1. Enter the URLs\n");
    printf("\t\t2. Download all URLs\n");
    printf("\t\t3. Download a URL\n");
    printf("\t\t4. Delete a URL\n");
    printf("\t\t5. Exit\n");
    printf("=======================================================================\n");
    printf("\t\tEnter your choice: ");
}

void get_URL()
{
    system("clear");
    printf("\n============================== WARNING ==================================\n");
    printf("\n======== USE DIRECT DOWNLOAD LINKS/URLs OTHERWISE ERROR OCCURS ==========\n");
    printf("\n=========================================================================\n");
    printf("\n=============== ENTER URL SECTION (MAX URL CAPACITY = 10) ===============\n");

    if (url_count >= MAX_URLS)
    {
        printf("URL limit reached. Cannot add more URLs.\n");
        return;
    }

    printf("Enter URL #%d: ", url_count + 1);
    scanf(" %[^\n]", URLs[url_count]); // read URL including spaces
    url_count++;

    printf("URL saved successfully!\n");
}

void show_all_URL()
{
    system("clear");
    printf("\n=============== SHOW ALL URLs SECTION ===============\n");

    if (url_count == 0)
    {
        printf("No URLs entered yet.\n");
        return;
    }

    printf("\nList of URLs:\n");
    for (int i = 0; i < url_count; ++i)
    {
        printf("%d. %s\n", i + 1, URLs[i]);
    }
}

void download_URL(int URL_num)
{
    system("clear");
    printf("\n=============== DOWNLOAD URL SECTION ===============\n");

    char output_filename[1000];
    int num_parts;
    pthread_t progress_thread;

    printf("Enter number of chunks (threads): ");
    scanf("%d", &num_parts);

    printf("Enter output file name: ");
    scanf(" %[^\n]", output_filename);

    curl_global_init(CURL_GLOBAL_ALL);

    const char *url = URLs[URL_num]; // Get the URL from the global list

    // Get total file size first
    total_file_size = get_file_size(url);

    // Start a thread to display download progress
    pthread_create(&progress_thread, NULL, progress_display, NULL);

    // Divide the file into chunks and start downloading
    divide_file_into_chunks(url, num_parts);

    // Merge all parts after download
    merge_parts(output_filename, num_parts);

    pthread_join(progress_thread, NULL);

    curl_global_cleanup();

    printf("Download completed and merged to '%s'.\n", output_filename);
}

void delete_URL(int URL_num)
{
    system("clear");
    printf("\n=============== DELETE URL SECTION ===============\n");

    if (URL_num < 0 || URL_num >= url_count)
    {
        printf("Invalid URL number. Cannot delete.\n");
        return;
    }

    // Shift all URLs after the deleted one to the left
    for (int i = URL_num; i < url_count - 1; ++i)
    {
        strcpy(URLs[i], URLs[i + 1]);
    }

    url_count--; // Decrease the count
    printf("URL deleted successfully.\n");
}

void menu_run()
{
    int choice;
    while (1)
    {
        system("clear");
        menu(); // clear and show the menu

        if (scanf("%d", &choice) != 1)
        {
            printf("Invalid input. Exiting.\n");
            break;
        }

        switch (choice)
        {
        case 1:
            get_URL();
            sleep(2);
            break;

        case 2:
            system("clear");
            printf("\n=============== DOWNLOAD ALL URLs SECTION ===============\n");

            if (url_count == 0)
            {
                printf("No URLs to download.\n");
            }
            else
            {
                int tempCount = url_count;
                for (int i = 0; i < url_count; i++)
                {
                    total_downloaded = 0;
                    total_file_size = 0;
                    download_URL(i);
                    printf("\n\n%d - URL content downloaded successfully!\n\n", i + 1);
                    sleep(3);
                }

                for (int i = 0; i < tempCount; i++)
                {
                    printf("Removing the links which are downloaded Successfully..\n");
                    sleep(3);
                    delete_URL(0);
                }
                
            }
            sleep(3);
            break;

        case 3:
        {
            int URL_num;
            show_all_URL();
            printf("\nEnter the number to download: ");
            if (scanf("%d", &URL_num) != 1 || URL_num < 1 || URL_num > url_count)
            {
                printf("Invalid URL number.\n");
                sleep(2);
                break;
            }
            total_downloaded = 0;
            total_file_size = 0;
            download_URL(URL_num - 1);
            sleep(3);
            delete_URL(URL_num - 1);
            sleep(3);
            break;
        }

        case 4:
        {
            int del_num;
            show_all_URL();
            printf("\nEnter the number to delete: ");
            if (scanf("%d", &del_num) != 1 || del_num < 1 || del_num > url_count)
            {
                printf("Invalid URL number.\n");
                sleep(2);
                break;
            }
            delete_URL(del_num - 1);
            sleep(2);
            break;
        }

        case 5:
            printf("\nExiting program. Goodbye!\n");
            exit(0);

        default:
            printf("Invalid choice. Please try again.\n");
            sleep(2);
            break;
        }
    }
}

int main()
{
    // Calling the menu to run
    menu_run();

    return 0;
}