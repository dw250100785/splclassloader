--TEST--
Instantiation SplClassLoader Class And Registe __Autoload() Functions
--FILE--
<?php
$loader = new SplClassLoader();

$loader->register();

var_dump($loader);

var_dump(spl_autoload_functions());

$loader->unregister();

var_dump(spl_autoload_functions());
?>
--EXPECT--
object(SplClassLoader)#1 (0) {
}
array(1) {
  [0]=>
  array(2) {
    [0]=>
    object(SplClassLoader)#1 (0) {
    }
    [1]=>
    string(9) "loadClass"
  }
}
array(0) {
}