fn cube_add(a: int, b: int) -> int
  c = a * a * a
  d = c + b
  return d
end

fn main()
  res = cube_add(10, 33)
  puts(str(res))  
end
