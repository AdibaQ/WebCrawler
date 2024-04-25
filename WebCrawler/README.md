# Dylan Granado 210005651
# Jessiah Martinez 201000443
# Adiba Qazi 200003688

## Architecture
This is a simple web crawler that acquires HTML content from web pages and extracts URLs to crawl further. 
It utilizes queues to manage the URLs and multithreading to process multiple URLs concurrently.

## Multithreading
Multithreading is achieved utilizing pthreads. A fixed number of threads (`NUM_THREADS`) are used to fetch URLs concurrently.
Each thread executes the `fetch_url` function, which dequeues URLs from the shared queue, fetches HTML content from the web, and checks it for more URLs.
Because multithreading is implemented, the crawling process is sped up since multiple URLs can be processed at once.

## Contributions/Specific Roles of each member

- **Dylan**: Development of the multithreading mechanism, implementing URL queueing, and the HTML parsing logic. Allowing the web crawler to fetch and process multiple URLs and enabling it to efficiently navigate web pages.

- **Jessiah**: Logged the progress of the web crawler, Allowing Multiple URLs as input

- **Adiba**: Allowing the web crawler to follow links within HTML documents, implemented depth control

## Libraries used
- `<stdio.h>`: Standard input and output operations.
- `<stdlib.h>`: Memory allocation/program termination.
- `<pthread.h>`: POSIX threads library for multithreading.
- `<string.h>`: String manipulation functions for working with URLs and HTML content.
- `<libxml/HTMLparser.h>`: Library for parsing HTML content.
- `<curl/curl.h>`: libcurl library for making HTTP requests and fetching HTML content from URLs.
