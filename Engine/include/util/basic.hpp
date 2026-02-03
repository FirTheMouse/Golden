#pragma once
#include <iostream>
#include <random>

template<typename... Args>
void print(Args&&... args) {
  (std::cout << ... << args) << std::endl;
}

template<typename... Args>
void printnl(Args&&... args) {
  (std::cout << ... << args);
}

inline std::string add_commas(int num) {
  std::string str = std::to_string(num);
  int insert_position = str.length() - 3;
  
  while(insert_position > 0) {
      str.insert(insert_position, ",");
      insert_position -= 3;
  }
  
  return str;
}

inline float randf(float min, float max)
{
    // One engine per thread; seeded once from real entropy.
    static thread_local std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);                  // [min, max) – max exclusive by default
}

inline int randi(int min, int max)
{
    static thread_local std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution<int> dist(min, max); // inclusive both ends
    return dist(rng);
}