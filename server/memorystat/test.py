class test:
    print("类初始化")
    a = 1
    b = {1}
    def __init__(self) -> None:
        self.__config = None
        print("类实例初始化")
        self.b.add(2)

    def __del__(self):
        print("类实例析构")
        print(f'b0:{self.b}')
        self.b = {3}
        # self.b.add(2)

        print(f'b0:{self.b}')
        
        print(f'b00:{test.b}')

# print(f'a:{test.a}')
print(f'b:{test.b}')
obj = test()
# print(f'a1:{test.a}')
# print(f'a2:{obj.a}')

print(f'b1:{test.b}')
print(f'b2:{obj.b}')
# del obj
obj = None
# print(f'a3:{test.a}')
# print(f'a4:{obj}')
print(f'b3:{test.b}')
print("结束运行")