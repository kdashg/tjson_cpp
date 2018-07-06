mkdir out 2>/dev/null
$CXX --std=c++14 rewrite_json.cpp tjson.cpp -o out/rewrite_json $@

