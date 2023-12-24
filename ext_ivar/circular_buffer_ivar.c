#include <ruby.h>

static ID ivar_buffer, ivar_length, ivar_read_cursor, ivar_write_cursor;

static VALUE circular_buffer_initialize(VALUE self, VALUE capacity_value)
{
    // Convert capacity_value from fixnum to C long
    long capacity = FIX2LONG(capacity_value);

    // 按容量初始化数组
    VALUE buffer = rb_ary_new_capa(capacity);
    rb_ary_resize(buffer, capacity);

    // 设置实例变量(instance_variable)
    rb_ivar_set(self, ivar_buffer, buffer); // 缓冲区数组
    rb_ivar_set(self, ivar_length, LONG2FIX(0)); // 使用长度
    rb_ivar_set(self, ivar_read_cursor, LONG2FIX(0)); // 读指针位置
    rb_ivar_set(self, ivar_write_cursor, LONG2FIX(0)); // 写指针位置

    return Qnil;
}

// 下一个指针位置
static long next_cursor_position(long capacity, long cursor)
{
    return (cursor + 1) % capacity;
}

// 读取
static VALUE circular_buffer_write(VALUE self, VALUE obj)
{
    // 获取实例变量 数组
    VALUE buffer = rb_ivar_get(self, ivar_buffer);
    // 数组长度
    long capacity = RARRAY_LEN(buffer);
    // 占用大小实例变量(转为Long)
    long length = FIX2LONG(rb_ivar_get(self, ivar_length));
    //写指针实例变量(转为Long)
    long write_cursor = FIX2LONG(rb_ivar_get(self, ivar_write_cursor));

    // 检查是否满了
    if (length == capacity) {
        rb_raise(rb_eRuntimeError, "Circular buffer is full");
    }

    // 数组写入传入的数据obj
    RARRAY_ASET(buffer, write_cursor, obj);
    // 下一个指针位置
    long next_write_cursor = next_cursor_position(capacity, write_cursor);
    // 设置 @write_cursor(转为ruby数字)
    rb_ivar_set(self, ivar_write_cursor, LONG2FIX(next_write_cursor));

    // 增加占用长度 @length
    rb_ivar_set(self, ivar_length, LONG2FIX(length + 1));

    // 返回写入的对象
    return obj;
}

// 读方法
static VALUE circular_buffer_read(VALUE self)
{
    // 获取数组实例变量
    VALUE buffer = rb_ivar_get(self, ivar_buffer);
    // 数组长度
    long capacity = RARRAY_LEN(buffer);
    // 占用大小
    long length = FIX2LONG(rb_ivar_get(self, ivar_length));
    // 读指针位置
    long read_cursor = FIX2LONG(rb_ivar_get(self, ivar_read_cursor));

    // 检查是否为空
    if (length == 0) {
        rb_raise(rb_eRuntimeError, "Circular buffer is empty"); // 抛异常
    }
    // 获取 数组指定元素
    VALUE obj = RARRAY_AREF(buffer, read_cursor);

    // 下一个读指针
    long next_read_cursor = next_cursor_position(capacity, read_cursor);
    // 设置 @read_cursor
    rb_ivar_set(self, ivar_read_cursor, LONG2FIX(next_read_cursor));

    // 增加占用长度
    rb_ivar_set(self, ivar_length, LONG2FIX(length - 1));

    // 返回获取的数据
    return obj;
}

void Init_circular_buffer_ivar(void)
{
    ivar_buffer = rb_intern("@buffer"); //数组 实例变量表示
    ivar_length = rb_intern("@length"); // 占用长度
    ivar_read_cursor = rb_intern("@read_cursor"); // 读指针位置
    ivar_write_cursor = rb_intern("@write_cursor"); // 写指针位置

    // 定义类 class CircularBufferIvar < Object
    VALUE cCircularBufferIvar = rb_define_class("CircularBufferIvar", rb_cObject);

    // initiazlize 方法
    rb_define_method(cCircularBufferIvar, "initialize", circular_buffer_initialize, 1);
    // 写方法（1个参数）
    rb_define_method(cCircularBufferIvar, "write", circular_buffer_write, 1);
    // 读取方法(无参数)
    rb_define_method(cCircularBufferIvar, "read", circular_buffer_read, 0);
}
