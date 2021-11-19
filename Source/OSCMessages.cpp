/*
        SuperColliderAU Copyright (c) 2006 Gerard Roma.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#include "OSCMessages.h"
OSCMessages::OSCMessages() {}

size_t OSCMessages::parameterMessage(small_scpacket *packet, string name,
                                     float value) {
  packet->reset();
  const char *buf = name.c_str();
  size_t nameSize = ((strlen(buf) + 4) >> 2) * 4;
  size_t messageSize = nameSize + 24;
  packet->adds("/n_set");
  packet->maketags(4);
  packet->addtag(',');
  packet->addtag('i');
  packet->addi(kDefaultNodeId);
  packet->addtag('s');
  packet->adds(buf);
  packet->addtag('f');
  packet->addf(value);
  return messageSize;
}

small_scpacket OSCMessages::sendTickMessage(int64 oscTime, int bus) {
  small_scpacket packet;
  packet.OpenBundle(oscTime);
  packet.BeginMsg();
  packet.adds("/c_set");
  packet.maketags(3);
  packet.addtag(',');
  packet.addtag('i');
  packet.addi(bus);
  packet.addtag('f');
  packet.addf(1.0);
  packet.EndMsg();
  packet.CloseBundle();
  return packet;
}

small_scpacket OSCMessages::initTreeMessage() {
  small_scpacket packet;
  packet.adds("/g_new");
  packet.maketags(2);
  packet.addtag(',');
  packet.addtag('i');
  packet.addi(1);
  return packet;
}

small_scpacket OSCMessages::quitMessage() {
  small_scpacket packet;
  packet.adds("/quit");
  return packet;
}

small_scpacket OSCMessages::freeAllMessage() {
  small_scpacket packet;
  packet.adds("/g_freeAll");
  packet.maketags(2);
  packet.addtag(',');
  packet.addtag('i');
  packet.addi(0);
  packet.EndMsg();
  return packet;
}

size_t OSCMessages::createSynthMessage(small_scpacket *packet, string name) {
  packet->reset();
  const char *buf = name.c_str();
  size_t nameSize = ((strlen(buf) + 4) >> 2) * 4;
  size_t messageSize = nameSize + 16;
  packet->adds("/s_new");
  packet->maketags(3);
  packet->addtag(',');
  packet->addtag('s');
  packet->adds(buf);
  packet->addtag('i');
  packet->addi(kDefaultNodeId);
  return messageSize;
}

small_scpacket OSCMessages::noteMessage(int64 oscTime, int note, int velocity) {
  small_scpacket packet;
  packet.OpenBundle(oscTime);

  packet.BeginMsg();
  packet.adds("/n_set");
  packet.maketags(4);
  packet.addtag(',');
  packet.addtag('i');
  packet.addi(kDefaultNodeId);
  packet.addtag('s');
  packet.adds("/note");
  packet.addtag('i');
  packet.addi(note);
  packet.EndMsg();

  packet.BeginMsg();
  packet.adds("/n_set");
  packet.maketags(4);
  packet.addtag(',');
  packet.addtag('i');
  packet.addi(kDefaultNodeId);
  packet.addtag('s');
  packet.adds("/velocity");
  packet.addtag('i');
  packet.addi(velocity);
  packet.EndMsg();

  packet.CloseBundle();
  return packet;
}
