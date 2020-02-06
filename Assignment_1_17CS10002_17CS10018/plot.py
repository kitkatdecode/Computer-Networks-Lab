import matplotlib.pyplot as plt

x_axis = [64,128,256,512,1024,2048]
y_axis = [58,112,221,439,874,1745]


plt.plot(x_axis,y_axis,marker='o')
plt.xlabel("Bandwidth (in Kbps)")
plt.ylabel("Number of UDP packets (in 10 sec transmision)")
plt.title("UDP packets v/s bandwidth")
plt.show()

# y_axis = [58,112,221,439,874,1745]

# plt.plot(x_axis,y_axis)
# plt.xlabel("Bandwidth (in Kbps)")
# plt.ylabel("Number of UDP packets")
# plt.title("UDP packets v/s bandwidth")
# plt.show()