#include "../include/gtestrunner.h"

int main(int argc, char *argv[])
{
  int Rts_i;
  testing::InitGoogleTest(&argc, argv);
  //::testing::GTEST_FLAG(filter) = "System_Test.Rational";
  Rts_i = RUN_ALL_TESTS();

#if defined(NDEBUG) // We are in Release compil
#else
  std::string Buffer_S;
  std::cout << "\nPress any key followed by enter to to quit ..." << std::endl;
  std::getline(std::cin, Buffer_S);
#endif

  return Rts_i;
}
