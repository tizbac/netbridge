#!/usr/bin/python

import sys
from PyQt4 import QtGui, QtCore # importiamo i moduli necessari
import xmlrpclib
proxy = xmlrpclib.ServerProxy("http://localhost:8888")
class MainWindow(QtGui.QMainWindow):
  def btdisconnect_click(self):
    proxy.disconnect()
  def btconnect_click(self):
    r = self.tableWidget.currentRow()
    if r != -1:
      ssid = self.tableWidget.item(r,0).text()
      mac = self.tableWidget.item(r,2).text()
      (probed_router,probed_dns) = proxy.probe_ssid("wclient0",str(ssid))
      
      (router_ip,acc) = QtGui.QInputDialog.getText(self,"Inserire l'indirizzo ip del router:","Inserire l'indirizzo ip del router:",QtGui.QLineEdit.Normal,probed_router)
      if not acc:
        return
      
      (dnslist,acc) = QtGui.QInputDialog.getText(self,"Inserire l'indirizzo ip dei server dns:","Inserire l'indirizzo ip dei server dns:",QtGui.QLineEdit.Normal,",".join(probed_dns))
      if not acc:
        return

      print proxy.change_dns(str(dnslist).split(","))
      #print str(mac),str(ssid),str(unicode(router_ip, "UTF-8"))
      print proxy.connect(str(mac),str(ssid),str(router_ip))
      #print str(router_ip)
  def btscan_click(self):
    results = proxy.scan("wclient0")
    self.tableWidget.setRowCount(len(results))
    for i in range(0,len(results)):
      self.tableWidget.setItem(i,0,QtGui.QTableWidgetItem(results[i][1]))
      self.tableWidget.setItem(i,1,QtGui.QTableWidgetItem(results[i][2]))
      self.tableWidget.setItem(i,2,QtGui.QTableWidgetItem(results[i][0]))
      self.tableWidget.setItem(i,3,QtGui.QTableWidgetItem(results[i][3]))
      self.tableWidget.setItem(i,4,QtGui.QTableWidgetItem("Si" if results[i][4] else "No"))
  def __init__(self):
    QtGui.QMainWindow.__init__(self) # da porre sempre all'inizio
                                      # inizializza alcuni metodi importanti come resize
    cWidget = QtGui.QWidget(self)
    self.resize(800, 480) # ridimensiona la finestra
    self.setWindowTitle('NETBRIDGE')
    vlayout1 = QtGui.QVBoxLayout()
    hlayout1 = QtGui.QHBoxLayout()
    
    self.tableWidget = QtGui.QTableWidget(self)
    self.tableWidget.setColumnCount(5)
    self.tableWidget.setHorizontalHeaderLabels(["SSID","Livello Segnale","MAC","Canale","Cifrata"])
    self.tableWidget.setColumnWidth(0,150)
    self.tableWidget.setColumnWidth(1,120)
    self.tableWidget.setColumnWidth(2,200)
    self.tableWidget.setColumnWidth(3,100)
    self.tableWidget.setColumnWidth(4,100)
    self.tableWidget.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)
    self.tableWidget.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)
    self.tableWidget.setSelectionMode(QtGui.QAbstractItemView.SingleSelection)
    button = QtGui.QPushButton('Quit')
    btscan = QtGui.QPushButton('Scansione')
    btconnect = QtGui.QPushButton('Connetti')
    btdisconnect = QtGui.QPushButton('Disconnetti')
    
    hlayout1.addWidget(button)
    hlayout1.addWidget(btscan)
    hlayout1.addWidget(btconnect)
    hlayout1.addWidget(btdisconnect)
    vlayout1.addWidget(self.tableWidget)
    vlayout1.addLayout(hlayout1)

    self.statusBar().showMessage('Versione 0.1') 
    
    cWidget.setLayout(vlayout1)
    self.setCentralWidget(cWidget)

    self.connect(button, QtCore.SIGNAL('clicked()'), QtCore.SLOT('close()'))
    self.connect(btconnect,QtCore.SIGNAL('clicked()'), self.btconnect_click )
    self.connect(btscan,QtCore.SIGNAL('clicked()'), self.btscan_click )
    self.connect(btdisconnect,QtCore.SIGNAL('clicked()'), self.btdisconnect_click )
app = QtGui.QApplication(sys.argv)
main = MainWindow()
main.show()
sys.exit(app.exec_())