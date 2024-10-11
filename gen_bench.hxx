#ifndef PROJECTS_SILLY_GEN_BENCH_H
#define PROJECTS_SILLY_GEN_BENCH_H

#include <iostream>
#include <string_view>
#include <vector>
#include <random>
#include <chrono>

[[maybe_unused]] static auto
gen_rand_ints(std::vector<int>::size_type n, int max = 1'000'000)
	-> std::vector<int>
{
	std::mt19937 prng((std::random_device())());
	std::uniform_int_distribution<int> dist(0, max);
	std::vector<int> ret;
	ret.reserve(n);

	for (unsigned i = 0; i < n; ++i)
		ret.push_back(dist(prng));

	return ret;
}

[[maybe_unused]] static auto gen_seq_ints(int start, int end)
	-> std::vector<int>
{
	std::vector<int> ret;
	ret.reserve(end - start);

	for (auto i = start; i < end; ++i)
		ret.push_back(i);

	return ret;
}

class Timer
{
public:
	Timer() = default;
	Timer &start()
	{
		tbeg = std::chrono::steady_clock::now();
		return *this;
	}
	Timer &end()
	{
		tend = std::chrono::steady_clock::now();
		return *this;
	}

	void print(std::ostream &oss, std::string_view msg = "Task")
	{
		std::chrono::duration<double> t = (tend - tbeg);
		oss << msg << ": Time: " << t.count() << "s\n";
	}
	void print_nano(std::ostream &oss, std::string_view msg = "Task")
	{
		std::chrono::nanoseconds t = (tend - tbeg);
		oss << msg << ": Time: " << t.count() << "ns\n";
	}

private:
	std::chrono::time_point<std::chrono::steady_clock> tbeg;
	std::chrono::time_point<std::chrono::steady_clock> tend;
};

#endif // End gen_bench.hxx