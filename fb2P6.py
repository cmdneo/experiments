with open("re-fb.p6", "wb") as fw:
    fw.write(b"P6\n1920 1080\n255\n")

    with open("re-fb.raw", "rb") as f:
        while 1:
            b3 = f.read(3)
            _a = f.read(1)
            if not b3:
                break
            fw.write(b3)
        
