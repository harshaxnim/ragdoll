matrixSize=20

clothFile=open("autoCloth","w")

clothFile.write("# simple-cloth\n\n")
clothFile.write("# vertex count\n")
clothFile.write("vc "+str(matrixSize**2)+"\n")
clothFile.write("\n")

for i in range(matrixSize):
	for j in range(matrixSize):
		clothFile.write("v ")
		clothFile.write(str(j)+" ")
		clothFile.write(str(matrixSize-1-i)+"\n")

clothFile.write("\n\n")
clothFile.write("#constraints\n\n")

for i in range(matrixSize):
	for j in range(matrixSize-1):
		clothFile.write("c 2 ")
		clothFile.write(str(i*matrixSize+j)+" "+str(i*matrixSize+j+1)+"\n")

clothFile.write("\n")
for i in range(matrixSize-1):
	for j in range(matrixSize):
		clothFile.write("c 2 ")
		clothFile.write(str(i*matrixSize+j)+" "+str((i+1)*matrixSize+j)+"\n")

clothFile.write("\n")

for i in range(matrixSize):
	 clothFile.write("c 1 ")
	 clothFile.write(str(i)+"\n")

