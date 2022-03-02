#include <iostream>
#include <string>


int input_double(double* ref) {
    size_t marker = 0;
    std::string buffer;

    std::getline(std::cin, buffer);

    try {
        *ref = std::stod(buffer, &marker);
    } catch (const std::invalid_argument &e) {
        std::cerr << "ERROR: No numerical input." << std::endl;
        return 1;
    } catch (const std::out_of_range &e) {
        std::cerr << "ERROR: Overflow." << std::endl;
        return 1;
    }

    if (buffer[marker]) {
        std::cerr << "ERROR: Invalid number! Parsing error." << std::endl;
        return 1;
    }

    return 0;
}
int main() {
    double op1 = 0.0;
    double op2 = 0.0;
    double result = 0.0;
    int err_status = 0;
    std::string operation;

    std::cout << "Input first operand: ";
    err_status = input_double(&op1);
    if (err_status)
        return EXIT_FAILURE;

    std::cout << "Input second operand: ";
    err_status = input_double(&op2);
    if (err_status)
        return EXIT_FAILURE;

    std::cout << "Choose operation (add|sub|mul|div): ";
    std::getline(std::cin, operation);


    if (operation == "add") {
        result = op1 + op2;
    } else if (operation == "sub") {
        result = op1 - op2;
    } else if (operation == "mul") {
        result = op1 * op2;
    } else if (operation == "div") {
        if (op2 == 0.0) {
            std::cerr << "FP ERROR: Cannot divide by zero!" << std::endl;
            return EXIT_FAILURE;
        }
        result = op1 / op2;
    } else {
        std::cerr << "Invalid operation '" << operation << "'. No such operation" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Result: " << result << std::endl;

    return EXIT_SUCCESS;
}
