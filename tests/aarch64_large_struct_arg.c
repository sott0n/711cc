struct Large {
    long first;
    long second;
    long third;
};

long consume_large(struct Large);

int main(void) {
    struct Large value;
    return consume_large(value);
}
