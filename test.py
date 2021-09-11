from pwn import *

context.log_level = "error"

money = 0
time = 0

for i in range(50):
  print(i)
  s = process(f"./A < tester/input_{i}.txt > tester/output_{i}.txt", shell=True)
  r = s.recvall().decode()
  print(r)
  for l in r.split("\n"):
    if l.startswith("money:"):
      money += int(l.split()[1])
    if l.startswith("time:"):
      time = max(time, float(l.split()[1]))
print("sum(money):", money)
print("max(time):", time)
