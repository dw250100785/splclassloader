--TEST--
Use Local Library And Namespace Prefix
--FILE--
<?php
$loader = new SplClassLoader("Php", dirname(__FILE__));
$loader->register();

$aes = new PhpSecure\Crypt\AES();
var_dump(get_class($aes));

$storage = Cache\CachePool::factory(
                array(
                    'storage'   => 'redis',
                    'namespace' => 'xxx:',
                )
            );
var_dump($storage);

var_dump($loader->getLocalNamespace());

$loader->registerLocalNamespace("Cache,Input,,");
$loader->registerLocalNamespace(array('Cache', 'Input,,', "Output"));
var_dump($loader->getLocalNamespace());
var_dump($loader->isLocalNamespace("Php"));
?>
--EXPECT--
string(19) "PhpSecure\Crypt\AES"
object(Cache\Storage\Redis)#3 (3) {
  ["resource":protected]=>
  NULL
  ["namespace":protected]=>
  string(4) "xxx:"
  ["ttl":protected]=>
  int(0)
}
string(3) "Php"
string(22) "Php:Cache:Input:Output"
bool(true)