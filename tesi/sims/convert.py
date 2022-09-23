import readline


with open("phy-sim-diode.csv", "w") as out:
    out.write("t,v(tgt),v(txd),i(tgt),i(txd)\n")
    with open("phy-sim-diode.sim", "r") as inp:
        while True:
            lines = [inp.readline().strip().split("\t")[-1] for _ in range(5)]
            foo = inp.readline()
            if foo == "":
                break
            out.write(",".join(lines) + "\n")

