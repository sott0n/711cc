struct FloatValue {
    float value;
};

int consume_float_value(struct FloatValue);

int main(void) {
    struct FloatValue value;
    return consume_float_value(value);
}
