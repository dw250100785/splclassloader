--TEST--
SplClassLoader::setFileExtension(), SplClassLoader::getFileExtension(), SplClassLoader::getPath(),
SplClassLoader::isLocalNamespace(), SplClassLoader::clearLocalNamespace()
--FILE--
<?php
$loader = new SplClassLoader("Php,Cache,Xu,,,", dirname(__FILE__));
$loader->register();
$loader->setFileExtension(".php5");
var_dump($loader->getFileExtension());
var_dump($loader->getPath("PhpSecure\Net\SCP"));
var_dump($loader->isLocalNamespace("Xu"));
$loader->registerLocalNamespace(array("Xu", "Dian", "Yang"));
var_dump($loader->getLocalNamespace());
$loader->clearLocalNamespace();
var_dump($loader->isLocalNamespace("Xu"));
?>
--EXPECTF--
string(5) ".php5"
string(76) "%s/splclassloader/tests/PhpSecure/Net/SCP.php5"
bool(true)
string(22) "Php:Cache:Xu:Dian:Yang"
bool(false)
