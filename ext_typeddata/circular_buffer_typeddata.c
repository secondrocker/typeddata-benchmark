#include <ruby.h>

// typeddata object
struct circular_buffer {
    long capacity; // 容量
    long length; // 已存数据长度
    long read_cursor; // 读指针index
    long write_cursor; // 写指针index 
    VALUE *buffer; // 缓冲区指针(存ruby对象，类似数组)
};

// 获取下一个index
static long next_cursor_position(struct circular_buffer *circular_buffer, long cursor)
{
    // 已缓冲区尽头 从头开始
    if (cursor == circular_buffer->capacity - 1) {
        return 0;
    } else {
        // 下一个Index
        return cursor + 1;
    }
}

static void circular_buffer_mark(void *ptr)
{
    struct circular_buffer *circular_buffer = ptr;

    long cursor = circular_buffer->read_cursor;
    // 从读指针read_cursor 到 写指针 write_cursor  逐个标记
    for (long i = 0; i < circular_buffer->length; i++) {
        // 标记对象支持压缩，如不支持调用rb_gc_mark 标记对象为不支持移动，就可以不识闲
        rb_gc_mark_movable(circular_buffer->buffer[cursor]);

        cursor = next_cursor_position(circular_buffer, cursor);
    }
}

// 释放内存,ptr为typeddata_object
static void circular_buffer_free(void *ptr)
{
    struct circular_buffer *circular_buffer = ptr;
    // 释放缓冲区
    free(circular_buffer->buffer);
    // 释放 tpyed_data object
    xfree(circular_buffer);
}

// 计算typed_data object 占用的内存大小
static size_t circular_buffer_memsize(const void *ptr)
{
    const struct circular_buffer *circular_buffer = ptr;
    // 结构体大小 + 缓冲区大小
    return sizeof(struct circular_buffer) + (sizeof(VALUE) * circular_buffer->capacity);
}

// compact 后更新地址
static void circular_buffer_compact(void *ptr)
{
    struct circular_buffer *circular_buffer = ptr;

    long cursor = circular_buffer->read_cursor;
    // 从读指针read_cursor 到写指针 write_cursor 逐个更新引用位置
    for (long i = 0; i < circular_buffer->length; i++) {
        // rb_gc_location 接受一个参数并返回对象的新地址
        circular_buffer->buffer[cursor] = rb_gc_location(circular_buffer->buffer[cursor]);

        cursor = next_cursor_position(circular_buffer, cursor);
    }
}

// TypedData 对象的核心是rb_data_type_t结构。该结构体包含 Ruby 需要了解的有关该对象的所有信息。描述了ruby应该怎么管理typedata object
const rb_data_type_t circular_buffer_data_type = {
    // 结构体名称，为便于调试，一般与typeddata object 名字相同
    .wrap_struct_name = "circular_buffer",
    // 一些主要函数
    .function = {
        .dmark = circular_buffer_mark, // 标记是否支持compact
        .dfree = circular_buffer_free, // 释放内存
        .dsize = circular_buffer_memsize, // 计算大小，可选，对查找内存问题很有用
        .dcompact = circular_buffer_compact, // 压缩后更新地址，每次压缩完成时都会调用此函数，以更新可能已移动的对象的引用。
    },
};

// 构造对象，参数klass为类，返回 typeddata object
static VALUE circular_buffer_allocate(VALUE klass)
{
    struct circular_buffer *circular_buffer;
    // 创建 TypedData object，传入 类、typeddata类型，typeddata信息，描述数据，typeddata object 指针
    VALUE obj = TypedData_Make_Struct(klass, struct circular_buffer,
                                      &circular_buffer_data_type, circular_buffer);
    // 返回 typeddata object
    return obj;
}

// 初始化对象，self：ruby对象本身，capacity_value：传入的容量数字
static VALUE circular_buffer_initialize(VALUE self, VALUE capacity_value)
{
    struct circular_buffer *circular_buffer;
    // 获取到 typeddata 对象，传入ruby的对象，typeddata 对象指针，typedata描述，typeddata 指针
    TypedData_Get_Struct(self, struct circular_buffer,
                         &circular_buffer_data_type, circular_buffer);

    // 把容量capacity_value转为c的long类型
    long capacity = FIX2LONG(capacity_value);
    // 写入 容量到 typeddata 对象结构体
    circular_buffer->capacity = capacity;
    // 缓冲区分配内存(按容量)
    circular_buffer->buffer = malloc(sizeof(VALUE) * capacity);

    // 与 ruby 的initialize方法一样，无需返回
    return Qnil;
}
// 缓冲区写入数据
static VALUE circular_buffer_write(VALUE self, VALUE obj)
{
    // 拿到 typeddata object
    struct circular_buffer *circular_buffer;
    TypedData_Get_Struct(self, struct circular_buffer,
                         &circular_buffer_data_type, circular_buffer);

    // 检查是否满了
    if (circular_buffer->length == circular_buffer->capacity) {
        // 抛异常(运行时异常)
        rb_raise(rb_eRuntimeError, "Circular buffer is full");
    }

    // 写入数据
    circular_buffer->buffer[circular_buffer->write_cursor] = obj;

    // 写指针指向下一个
    circular_buffer->write_cursor = next_cursor_position(circular_buffer, circular_buffer->write_cursor);
    // 已存数据长度 + 1
    circular_buffer->length++;

    return obj;
}

// 读取缓存区数据
static VALUE circular_buffer_read(VALUE self)
{
    // 获取 typeddata object
    struct circular_buffer *circular_buffer;
    TypedData_Get_Struct(self, struct circular_buffer,
                         &circular_buffer_data_type, circular_buffer);

    // 检查是否为空
    if (circular_buffer->length == 0) {
        rb_raise(rb_eRuntimeError, "Circular buffer is empty");
    }
    // 读取内容(读指针)
    VALUE obj = circular_buffer->buffer[circular_buffer->read_cursor];

    // 写指针指向下一个
    circular_buffer->read_cursor = next_cursor_position(circular_buffer, circular_buffer->read_cursor);
    // 占用长度 -1
    circular_buffer->length--;

    // 返回数据
    return obj;
}

void Init_circular_buffer_typeddata(void)
{
    // class CircularBufferTypedData < Object 定义类
    VALUE cCircularBufferTypedData = rb_define_class("CircularBufferTypedData", rb_cObject);

    // 定义分配函数，该函数返回typed_data对象(代替普通Object)，在initialize前调用，可视作覆盖new方法
    rb_define_alloc_func(cCircularBufferTypedData, circular_buffer_allocate);

    // 定义initialize方法,1个参数
    rb_define_method(cCircularBufferTypedData, "initialize", circular_buffer_initialize, 1);
    // 写数据方法
    rb_define_method(cCircularBufferTypedData, "write", circular_buffer_write, 1);
    // 读数据方法
    rb_define_method(cCircularBufferTypedData, "read", circular_buffer_read, 0);
}
