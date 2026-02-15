#include "credentials.hpp"
#include <boost/test/unit_test.hpp>
#include <sstream>

BOOST_AUTO_TEST_SUITE(credentials_suite)

BOOST_AUTO_TEST_CASE(credentials_test)
{
  std::ostringstream out;
  levkin::out_credentials(out);
  BOOST_TEST(out.str() == "levkin.dima");
}

BOOST_AUTO_TEST_CASE(yet_another_credentials_test)
{
  std::ostringstream out;
  levkin::out_credentials(out);
  BOOST_TEST(out.str()[0] == 'l');
}

BOOST_AUTO_TEST_SUITE_END()
