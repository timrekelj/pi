#include <filesystem>
#include <fstream>
#include <gmpxx.h>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

constexpr double DIGITS_PER_TERM = 14.181647462725477;

const mpz_class C3_OVER_24("10939058860032000");

struct Terms {
    mpz_class p;
    mpz_class q;
    mpz_class t;
};

Terms binary_split(long start, long end) {
    if (end - start == 1) {
        if (start == 0) {
            return {1, 1, 13591409};
        }

        mpz_class a = start;
        mpz_class p = (6 * a - 5) * (2 * a - 1) * (6 * a - 1);
        mpz_class q = a * a * a * C3_OVER_24;
        mpz_class t = p * (13591409 + 545140134 * a);

        if (start % 2 == 1) {
            t = -t;
        }

        return {p, q, t};
    }

    long mid = start + (end - start) / 2;
    Terms left = binary_split(start, mid);
    Terms right = binary_split(mid, end);

    Terms result;
    result.p = left.p * right.p;
    result.q = left.q * right.q;
    result.t = right.q * left.t + left.p * right.t;

    return result;
}

std::string calculate_pi_digits(long digits) {
    constexpr long guard_digits = 10;
    long scaled_digits = digits + guard_digits;
    long terms_count = static_cast<long>(scaled_digits / DIGITS_PER_TERM) + 2;

    std::cerr << "Using " << terms_count << " Chudnovsky terms...\n";
    Terms terms = binary_split(0, terms_count);

    mpz_class scale;
    mpz_ui_pow_ui(scale.get_mpz_t(), 10, scaled_digits);

    mpz_class sqrt_input = 10005 * scale * scale;
    mpz_class root;
    mpz_sqrt(root.get_mpz_t(), sqrt_input.get_mpz_t());

    mpz_class numerator = terms.q;
    mpz_class multiplier = 426880;
    mpz_class denominator = terms.t;
    mpz_class common;

    mpz_gcd(common.get_mpz_t(), numerator.get_mpz_t(), denominator.get_mpz_t());
    numerator /= common;
    denominator /= common;

    mpz_gcd(common.get_mpz_t(), multiplier.get_mpz_t(), denominator.get_mpz_t());
    multiplier /= common;
    denominator /= common;

    mpz_gcd(common.get_mpz_t(), root.get_mpz_t(), denominator.get_mpz_t());
    root /= common;
    denominator /= common;

    mpz_class pi = (numerator * multiplier * root) / denominator;
    std::string result = pi.get_str();

    if (static_cast<long>(result.size()) < digits) {
        throw std::runtime_error("calculated fewer digits than expected");
    }

    result.resize(digits);
    return result;
}

long parse_positive_long(const char* value, const char* name) {
    std::string text(value);
    size_t parsed = 0;
    long number = std::stol(text, &parsed);

    if (parsed != text.size() || number < 1) {
        throw std::runtime_error(std::string(name) + " must be a positive integer");
    }

    return number;
}

int main(int argc, char* argv[]) {
    try {
        long total_digits = argc > 1 ? parse_positive_long(argv[1], "totalDigits") : 1000000;
        long chunk_size = argc > 2 ? parse_positive_long(argv[2], "chunkSize") : 100000;
        fs::path output_dir = argc > 3 ? fs::path(argv[3]) : fs::path("pi");

        fs::create_directories(output_dir);

        std::cerr << "Calculating " << total_digits << " pi digits...\n";
        std::string digits = calculate_pi_digits(total_digits);

        for (long start = 0, index = 0; start < total_digits; start += chunk_size, index++) {
            long length = std::min(chunk_size, total_digits - start);
            fs::path file = output_dir / (std::to_string(index) + ".txt");
            std::ofstream output(file, std::ios::binary);

            if (!output) {
                throw std::runtime_error("could not write " + file.string());
            }

            output.write(digits.data() + start, length);
            output.put('\n');
            std::cerr << "Wrote " << file << " (" << length << " digits)\n";
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
