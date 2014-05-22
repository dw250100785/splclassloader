--TEST--
Set Local Library
--FILE--
<?php
$loader = new SplClassLoader(null, dirname(__FILE__));
$loader->register();
// use current dir
$aes = new PhpSecure\Crypt\AES();
var_dump(get_class($aes));
var_dump(new Phpmq\Queue());
$loader->setLibraryPath(dirname(dirname(__FILE__)));
var_dump($loader->getLibraryPath());
new Xu\Dian();

?>
--EXPECTF--
string(19) "PhpSecure\Crypt\AES"
object(Phpmq\Queue)#3 (3) {
  ["a"]=>
  string(1) "b"
  ["c":"Phpmq\Queue":private]=>
  string(1) "d"
  ["e":protected]=>
  int(33234)
}
string(47) "%s"

Warning: SplClassLoader::loadClass(): Failed opening script %s: No such file or directory in %s on line %d

Fatal error: Class 'Xu\Dian' not found in %s on line %d