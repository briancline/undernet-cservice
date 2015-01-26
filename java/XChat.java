// XChat.java

// Undernet Channel Service (X)
// Copyright (C) 1995-2002 Robin Thellend
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// The author can be contact by email at <csfeedback@robin.pfft.net>
//
// Please note that this software is unsupported and mostly
// obsolete. It was replaced by GNUworld/CMaster. See
// http://gnuworld.sourceforge.net/ for more information.
//

import java.awt.*;
import java.applet.Applet;
import java.io.*;
import java.net.*;

public class XChat extends Applet {
  static Connection sock;
  static TextField Input;
  static TextArea Output;
  static Button ConnectButton;
  static Button DisconnectButton;

  public void init() {
    sock = null;
    Input = new TextField(50);
    Output = new TextArea(10, 50);
    ConnectButton = new Button("Connect");
    DisconnectButton = new Button("Disconnect");

    DisconnectButton.disable();

    Output.setEditable(false);

    setLayout(new FlowLayout(FlowLayout.CENTER));

    add(ConnectButton);
    add(DisconnectButton);
    add(Output);
    add(Input);

    validate();
  }

  public boolean action(Event evt, Object arg) {
    if(evt.target == Input) {
      if(sock == null) {
        Output.appendText("### Not connected!\n");
        Input.selectAll();
      }
      else if(sock.status == 0) {  // Need Username
        sock.username = Input.getText();
        Output.appendText(sock.username + "\nPassword: ");
        Input.setText("");
        sock.status = 1;
      }
      else if(sock.status == 1) {  // Need Password
        sock.password = Input.getText();
        Input.setText("");
        Output.appendText("\nConnecting...\n");
        Input.selectAll();
        sock.status = 2;
        try {
          sock.os.writeBytes("CHAT " + sock.username + "/" +
                             sock.password + "\n");
        }
        catch (IOException e) {
          Output.appendText("Connection failed!\n");
          try {
            sock.socket.close();
          }
          catch (IOException ee) {
            Output.appendText("### Error closing socket!\n");
          }
          sock = null;
        }
      }
      else {  // Active connection
        try {
          sock.os.writeBytes(Input.getText() + "\n");
        }
        catch (IOException e) {
          LostConn();
        }
        Input.selectAll();
      }
      return true;
    }
    else if(evt.target == ConnectButton) {
      ConnectButton.disable();
      DisconnectButton.enable();
      sock = new Connection(getCodeBase().getHost(),7357);
      return true;
    }
    else if(evt.target == DisconnectButton) {
      LostConn();

      return true;
    }
        
    return false;
  }

  static public void LostConn() {
    ConnectButton.enable();
    DisconnectButton.disable();
    if(sock != null) {
      sock.close();
      sock = null;
    }
  }
  public void destroy() {
    if(sock!= null) {
      sock.close();
      sock = null;
    }
  }
}


class Connection {
  Socket socket;
  TextArea Output;
  ReadFromServer read;
  DataOutputStream os;
  public int status;
  String username;
  String password;

  Connection(String host, int port) {
    try {
      Output = XChat.Output;
      socket = new Socket(host,port);
      status = 0;
      os = new DataOutputStream(socket.getOutputStream());
      read = new ReadFromServer(socket);
      read.start();
      Output.appendText("Username: ");
    }
    catch (UnknownHostException e1) {
      Output.appendText("### Can't resolve host name!\n");
    }
    catch (IOException e2) {
      Output.appendText("### Can't connect!\n");
    }
  }

  public void close() {
    try {
      socket.close();
      Output.appendText("### Disconnected!\n");
    }
    catch (IOException e) {
      Output.appendText("### Error closing socket\n");
    }
    read.stop();
  }
}


class ReadFromServer extends Thread {
  DataInputStream is;
  Socket sock;
  TextArea Output;

  ReadFromServer(Socket sock) {
    try {
      is = new DataInputStream(sock.getInputStream());
      this.Output = XChat.Output;
      this.sock = sock;
    }
    catch (IOException e) {
      try {
        is.close();
        sock.close();
      }
      catch (IOException ee) {
        Output.appendText("### Error closing input stream!\n");
      }
      Output.appendText("### Could not open input stream!\n");
      stop();
    }
  }
  public void run() {
    while(true){
      try {
        String in = is.readLine();
        if(in == null){
          XChat.LostConn();
          stop();
        }
        Output.appendText(in + "\n");
      }
      catch (IOException e) {
        try {
          is.close();
          sock.close();
        }
        catch (IOException ee) {
          Output.appendText("### Error closing input stream!\n");
        }
        stop();
      }
    }
  }
}

