data = open("data.inc").read()
data = "".join(data.split()).split(",")

flag = ""
for k in range(len(data)):
    want = data[k].count("z(")
    for c in range(256):
        if k % 7 == 0: r = c ^ 0x63
        if k % 7 == 1: r = c ^ 48
        if k % 7 == 2: r = c << 4
        if k % 7 == 3: r = c << 2
        if k % 7 == 4: r = c * 55
        if k % 7 == 5: r = c + 97
        if k % 7 == 6: r = c + 44
        if r == want:
            flag += chr(c)
    if flag.endswith("\n"):
        break

print(flag)