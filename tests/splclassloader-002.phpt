--TEST--
SplClassLoader Manual Load Class 
--FILE--
<?php
$loader = new SplClassLoader(null, dirname(__FILE__));

$loader->loadClass('PhpSecure\File\ANSI');
$ansi = new PhpSecure\File\ANSI();
var_dump(get_class($ansi));
?>
--EXPECT--
string(19) "PhpSecure\File\ANSI"