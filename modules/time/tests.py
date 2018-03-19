import time
class A(object):
	def __init__(self, b):
		self.a = b

	def b(self):
		self.a = self.a + 1

print(time.time())

a = 1
n = range(1,100000)
print(time.time())
for i in n:
	b = A(i)
	#a.b()
	a += b.a

print(a, b)
print(time.time())