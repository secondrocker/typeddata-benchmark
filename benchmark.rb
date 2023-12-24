require_relative "circular_buffer_ruby"
require_relative "circular_buffer_ivar"
require_relative "circular_buffer_typeddata"

require "benchmark/ips"

SIZE = 1000

# 读写1000次
def benchmark_circular_buffer(klass)
  circular_buffer = klass.new(SIZE)

  SIZE.times do |i|
    circular_buffer.write(i)
  end

  SIZE.times do |i|
    circular_buffer.read
  end
end

Benchmark.ips do |x|
  # ruby 实现
  x.report("circular_buffer_ruby") do
    benchmark_circular_buffer(CircularBufferRuby)
  end

  # 通过操作实例变量实现
  x.report("circular_buffer_ivar") do
    benchmark_circular_buffer(CircularBufferIvar)
  end

  # typeddata 实现
  x.report("circular_buffer_typeddata") do
    benchmark_circular_buffer(CircularBufferTypedData)
  end

  x.compare!
end
