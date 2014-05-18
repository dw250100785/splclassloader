# SplClassLoader PHP Extension

##PHP自动加载器

最初的PHP实现：

[https://gist.github.com/jwage/221634](https://gist.github.com/jwage/221634)

最初的C实现：

[https://github.com/metagoto/splclassloader](https://github.com/metagoto/splclassloader)

好几年前的C实现，其实还可以。但是作为强迫症的我，还是想自己重构一下。将原来的C实现修改了大部分，比如，类库分为本地类库，全局类库，可注册本地命名空间前缀，可ini中配置加载的文件扩展名和全局类库地址，去掉SPL查找include_path的问题，这也是受到[Yaf](https://github.com/laruence/php-yaf)实现的影响吧。

之所以要去实现psr-0的自动加载，希望开发都按着此规范，并且自动加载逻辑一般不会修改。由于在想做其他东西的时候，没有一个好的自动加载的，如果PHP去实现，loader还是要include一次，强迫症就来了，呵呵。

虽然是fork原来C版本的实现，但是为了尊重原作者，就这样吧，但是就不pull request了，我想作者也不会合并吧。

关于psr-0的规范

[https://github.com/hfcorriez/fig-standards/blob/zh_CN/%E6%8E%A5%E5%8F%97/PSR-0.md](https://github.com/hfcorriez/fig-standards/blob/zh_CN/%E6%8E%A5%E5%8F%97/PSR-0.md)

### 要求

* PHP 5.2 +


###编译安装


#### Linux/Unix

	$ cd splclassloader
	$/path/to/phpize
	$./configure --with-php-config=/path/to/php-config
	$make && make install
	
### 作者

This project is authored and maintained by [xudianyang](http://www.phpboy.net/).
	

### 自动加载器SplClassLoader原型

[SplClassLoader Auto Complete](https://github.com/xudianyang/yaf.auto.complete/blob/master/SplClassLoader.php)

```php

<?php
class SplClassLoader
{
    /**
     * 构造方法，实例化一个自动加载器
     *
     * @param $namespace string 命名空间前缀（多个前缀以英文逗号分隔）
     * @param $local_library_path string 本地类库全路径
     */
    public function __construct($namespace, $local_library_path){}

    /**
     * 将SplClassLoader注册到SPL __autoload函数栈中
     *
     * @return Boolean
     */
    public function register(){}

    /**
     * 将SplClassLoader从到SPL __autoload函数栈中移出
     *
     * @return Boolean
     */
    public function unregister(){}

    /**
     * 装载指定的类
     *
     * @param $class_name string 类名（带命名空间分隔符或者_）
     *
     * @return Boolean
     */
    public function loadClass($class_name){}

    /**
     * 设置全局或者本地类库的路径
     *
     * @param $path string 类库全路径名（如果设置全局类库路径会覆盖ini的splclassloader.global_library的值）
     * @param $global Boolean false则设置本地类库路径，true则设置全局类库路径
     *
     * @return SplClassLoader
     */
    public function setLibraryPath($path, $global = false){}

    /**
     * 获取全局或者本地类库的路径
     *
     * @param $global Boolean false则设置本地类库路径，true则设置全局类库路径
     *
     * @return SplClassLoader
     */
    public function getLibraryPath($global = false){}

    /**
     * 设置PHP文件扩展名（会覆盖ini中设置的splclassloader.ext值）
     *
     * @param $ext
     *
     * @return SplClassLoader
     */
    public function setFileExtension($ext){}

    /**
     * 获取当前自动加载器实例的PHP文件扩展名
     *
     * @return string
     */
    public function getFileExtension(){}

    /**
     * 获取指定类名的文件全路径
     * 
     * @param $class_name string 类名（带命名空间分隔符或者_）
     *
     * @return string
     */
    public function getPath($class_name){}

    /**
     * 获取当前自动加载器实例的本地类的命名空间前缀
     *
     * @return string
     */
    public function getLocalNamespace(){}

    /**
     * 注册本地命名空间前缀
     *
     * @param $namespace_prefix mixed 命名空间前缀（可以为数组或者英文逗号分隔的字符串）
     *
     * @return SplClassLoader
     */
    public function registerLocalNamespace($namespace_prefix){}

    /**
     * 判断前缀是否为已注册的本地命名空间前缀
     *
     * @param $namespace string 命名空间前缀
     *
     * @return Boolean
     */
    public function isLocalNamespace($namespace){}

    /**
     * 清楚已注册的本地命名空间前缀
     *
     * @return Boolean
     */
    public function clearLocalNamespace(){}
}

```

### 自动加载策略（与psr-0的差异）

除了我们在项目开发中需要按照psr-0建立目录和定义类以外，还需要注意C扩展实现的loader的一些特殊加载策略。

1.本地类库的路径（实例化loader或者setLibraryPath指定）与全局类库的路径两者必须设置其一。

2.当设置本地类库路径时，未注册任何的本地命名空间前缀，所有类都到本地类库中加载。

3.当设置本地类库路径时，注册了本地命名空间前缀

	a.类名中如果包含已注册的前缀则到本地类库中加载；
	b.类中没有包含已注册的前缀，首先判断全局类库路径是否已设置，如果是则到全局类库中加载，否则到本地类库加载。

4.当注册本地命名空间前缀，但是未设置本地类库路径时，加载失败。

5.当未设置本地库路径和未注册本地命名空间前缀，是到全局类库路径中加载类。

