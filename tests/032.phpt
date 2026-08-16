--TEST--
Yaconf: sparse tables keep every key reachable after the compacted hash region is rebuilt
--INI--
yaconf.directory={PWD}/inis/032/
--FILE--
<?php
// inis/032/sparse.ini yields a 22-key top-level table (k01..k20, sub, num).
// Parsing allocates tables at nTableSize=128, so this table has 22 buckets in
// a 128-slot region — the compact pass used to shrink-and-rehash such tables;
// it now copies the full region as-is.  Either way every key must stay
// reachable and iteration must keep insertion order (bucket order is copied
// as-is, only the hash slots move).
for ($i = 1; $i <= 20; $i++) {
    printf("k%02d=%s\n", $i, Yaconf::get(sprintf("sparse.k%02d", $i)));
}
var_dump(Yaconf::get("sparse.sub"));
var_dump(Yaconf::get("sparse.num"));
var_dump(array_keys(Yaconf::get("sparse")) === array_merge(
    array_map(function($i){ return sprintf("k%02d", $i); }, range(1, 20)),
    ["sub", "num"]
));
var_dump(Yaconf::has("sparse.k01"));
var_dump(Yaconf::has("sparse.nope"));
?>
--EXPECT--
k01=v01
k02=v02
k03=v03
k04=v04
k05=v05
k06=v06
k07=v07
k08=v08
k09=v09
k10=v10
k11=v11
k12=v12
k13=v13
k14=v14
k15=v15
k16=v16
k17=v17
k18=v18
k19=v19
k20=v20
array(2) {
  ["a"]=>
  string(1) "1"
  ["b"]=>
  string(1) "2"
}
array(1) {
  [0]=>
  string(4) "zero"
}
bool(true)
bool(true)
bool(false)
