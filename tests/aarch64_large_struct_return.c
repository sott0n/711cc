struct Large {
    long first;
    long second;
    long third;
};

struct Large make_large(void);

int main(void) {
    return make_large().first;
}
