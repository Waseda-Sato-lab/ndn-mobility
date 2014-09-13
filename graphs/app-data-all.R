#!/usr/bin/Rscript

# app-data-all.R
# Simple R script to make graphs from ndnSIM tracers - App delay
#
# Copyright (c) 2014 Waseda University, Sato Laboratory
# Author: Jairo Eduardo Lopez <jairo@ruri.waseda.jp>
#
# app-data-all.R is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# app-data-j.R is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of              
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               
# GNU Affero Public License for more details.                                 
#                                                                             
# You should have received a copy of the GNU Affero Public License            
# along with app-data-all.R.  If not, see <http://www.gnu.org/licenses/>.
#                    

suppressPackageStartupMessages(library (ggplot2))
suppressPackageStartupMessages(library (scales))
suppressPackageStartupMessages(library (optparse))
suppressPackageStartupMessages(library (doBy))
suppressPackageStartupMessages(library (plyr))

data.mobile1 = read.table ("/home/jelfn/git/ndn-mobility/results/NDNMobilityRandom-app-delays-flood-04-001-054.txt", header=T)
data.mobile2 = read.table ("/home/jelfn/git/ndn-mobility/results/NDNMobilityRandom-app-delays-smart-04-001-054.txt", header=T)

data.mobile1$TimeSec = 1 * ceiling (data.mobile1$Time)
data.mobile2$TimeSec = 1 * ceiling (data.mobile2$Time)

data.mobile1$Node = factor (data.mobile1$Node)
data.mobile2$Node = factor (data.mobile2$Node)

data.mobile1$Type = factor (data.mobile1$Type)
data.mobile2$Type = factor (data.mobile2$Type)

mobile1name = "Flooding"
mobile2name = "Smart Flooding"

mobile1.combined = summaryBy (. ~ TimeSec + Type, data=data.mobile1, FUN=mean)
mobile2.combined = summaryBy (. ~ TimeSec + Type, data=data.mobile2, FUN=mean)

mobile1.combined$Variable = "flood"
mobile2.combined$Variable = "smart"

combinedlist = list(mobile1.combined, mobile2.combined)
allcombineddata = do.call(rbind.fill, combinedlist)

satTime = subset(allcombineddata, Type %in% c("FullDelay"))

g.all <- ggplot (satTime, aes(colour=Variable)) +
  geom_line (aes (x=TimeSec, y=DelayS.mean), size=1) +
  ggtitle ("Average Delay time for 4 mobile nodes") +
  ylab ("Delay [Seconds]") +
  xlab ("Simulation Time [Seconds]") +
  scale_colour_discrete(name = "NDN Forwarding strategy",
                        labels = c(mobile1name, mobile2name))

print (g.all)

g.all <- ggplot (satTime, aes(colour=Variable)) +
  geom_line (aes (x=TimeSec, y=HopCount.mean), size=1) +
  ggtitle ("Average Packet Hop Count for 4 mobile nodes") +
  ylab ("Hop Count") +
  xlab ("Simulation Time [Seconds]") +
  scale_colour_discrete(name = "NDN Forwarding strategy",
                        labels = c(mobile1name, mobile2name))

print (g.all)