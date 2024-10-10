#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "tinyxml2.h"

// Structure to hold the response data
struct MemoryStruct {
    char *memory;
    size_t size;
};

// Callback function to write data received by CURL into a memory buffer
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = (char *)realloc(mem->memory, mem->size + real_size + 1); // Cast added here
    if(ptr == NULL) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, real_size);
    mem->size += real_size;
    mem->memory[mem->size] = 0;

    return real_size;
}

// Function to perform the HTTP request
void perform_request(const char* url, const char* username, const char* password, struct MemoryStruct *chunk) {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        
        // Set basic authentication
        char userpwd[256];
        snprintf(userpwd, sizeof(userpwd), "%s:%s", username, password);
        curl_easy_setopt(curl, CURLOPT_USERPWD, userpwd);

        // Set callback function to capture response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);

        // Enable verbose output for debugging
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        // Set a timeout to avoid hanging
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        // Perform the request
        res = curl_easy_perform(curl);

        // Check if the request was successful
        if (res != CURLE_OK) {
            fprintf(stderr, "Request failed: %s\n", curl_easy_strerror(res));
        } else {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            printf("HTTP response code: %ld\n", response_code);

            if (response_code == 200) {
                printf("Command executed successfully!\n");
            } else {
                fprintf(stderr, "Unexpected response from the server. HTTP Status Code: %ld\n", response_code);
            }
        }

        // Cleanup
        curl_easy_cleanup(curl);
    } else {
        fprintf(stderr, "Error initializing CURL.\n");
    }
}

// Function to fetch the status from the device and parse XML
void fetch_status(const char* device_ip, const char* username, const char* password) {
    struct MemoryStruct chunk;
    chunk.memory = (char *)malloc(1); // Cast added here
    chunk.size = 0;

    char url[256];
    snprintf(url, sizeof(url), "http://%s/status.xml", device_ip);

    perform_request(url, username, password, &chunk);

    // Output the received status.xml content
    printf("Received XML:\n%s\n", chunk.memory);

    // Parse XML using tinyxml2
    tinyxml2::XMLDocument doc;
    if (doc.Parse(chunk.memory) == tinyxml2::XML_SUCCESS) {
        tinyxml2::XMLElement* monitor = doc.FirstChildElement("Monitor");
        if (monitor) {
            const char* device = monitor->FirstChildElement("Device")->GetText();
            const char* analogInput1 = monitor->FirstChildElement("AnalogInput1")->GetText();
            const char* analogInput2 = monitor->FirstChildElement("AnalogInput2")->GetText();
            const char* digitalInput1 = monitor->FirstChildElement("DigitalInput1")->GetText();
            const char* digitalInput2 = monitor->FirstChildElement("DigitalInput2")->GetText();
            const char* relay1 = monitor->FirstChildElement("Relay1")->GetText();
            const char* relay2 = monitor->FirstChildElement("Relay2")->GetText();

            printf("Device: %s\n", device);
            printf("Analog Input 1: %s\n", analogInput1);
            printf("Analog Input 2: %s\n", analogInput2);
            printf("Digital Input 1: %s\n", digitalInput1);
            printf("Digital Input 2: %s\n", digitalInput2);
            printf("Relay 1: %s\n", relay1);
            printf("Relay 2: %s\n", relay2);
        } else {
            printf("Failed to find <Monitor> element in the XML\n");
        }
    } else {
        printf("Failed to parse XML\n");
    }

    free(chunk.memory);
}

// Main function for testing
int main() {
    const char* device_ip = "192.168.1.2";
    const char* username = "admin";
    const char* password = "admin";

    // Fetch the device status
    fetch_status(device_ip, username, password);

    return 0;
}
