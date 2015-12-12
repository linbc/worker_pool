worker_pool_test:
    g++ -g -std=c++11 -D_GLIBCXX_USE_NANOSLEEP -lpthread -o worker_pool_test worker_pool_test.cc
clean:
    rm -rf worker_pool_test
