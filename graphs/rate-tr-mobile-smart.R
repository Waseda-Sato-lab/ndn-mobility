#!/usr/bin/Rscript

# Simple R script to make graphs from ndnSIM tracers - Aggregate throughput
# 2014-09-11
# Jairo Eduardo Lopez

# rate-tr-mobile-smart.R
# Script to make graphs from ndnSIM tracers - Compare by number of nodes
#
# Copyright (c) 2014 Waseda University, Sato Laboratory
# Author: Jairo Eduardo Lopez <jairo@ruri.waseda.jp>
#
# rate-tr-mobile-smart.R is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# rate-tr-mobile-smart.R is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of              
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               
# GNU Affero Public License for more details.                                 
#                                                                             
# You should have received a copy of the GNU Affero Public License            
# along with rate-tr-mobile-smart.R.  If not, see <http://www.gnu.org/licenses/>.
#                    

suppressPackageStartupMessages(library (ggplot2))
suppressPackageStartupMessages(library (scales))
suppressPackageStartupMessages(library (optparse))
suppressPackageStartupMessages(library (doBy))
suppressPackageStartupMessages(library(plyr))

# set some reasonable defaults for the options that are needed
option_list <- list (
  make_option(c("-o", "--output"), type="character", default=".",
              help="Output directory for graphs.\n\t\t[Default \"%default\"]")
)

# Load the parser
opt = parse_args(OptionParser(option_list=option_list, description="Creates graphs from ndnSIM L3 Data rate Tracer data"))

data.mobile1 = read.table ("/home/jelfn/git/ndn-mobility/results/NDNMobilityRandom-rate-trace-smart-01-001-054.txt", header=T)
data.mobile2 = read.table ("/home/jelfn/git/ndn-mobility/results/NDNMobilityRandom-rate-trace-smart-02-001-054.txt", header=T)
data.mobile3 = read.table ("/home/jelfn/git/ndn-mobility/results/NDNMobilityRandom-rate-trace-smart-03-001-054.txt", header=T)
data.mobile4 = read.table ("/home/jelfn/git/ndn-mobility/results/NDNMobilityRandom-rate-trace-smart-04-001-054.txt", header=T)

data.mobile1$Node = factor (data.mobile1$Node)
data.mobile2$Node = factor (data.mobile2$Node)
data.mobile3$Node = factor (data.mobile3$Node)
data.mobile4$Node = factor (data.mobile4$Node)

data.mobile1$Kilobits <- data.mobile1$Kilobytes * 8
data.mobile2$Kilobits <- data.mobile2$Kilobytes * 8
data.mobile3$Kilobits <- data.mobile3$Kilobytes * 8
data.mobile4$Kilobits <- data.mobile4$Kilobytes * 8

data.mobile1$Type = factor (data.mobile1$Type)
data.mobile2$Type = factor (data.mobile2$Type)
data.mobile3$Type = factor (data.mobile3$Type)
data.mobile4$Type = factor (data.mobile4$Type)

name = sprintf("Network average throughput on mobile simulation")

mobile1name = "1 mobile node"
mobile2name = "2 mobile nodes"
mobile3name = "3 mobile nodes"
mobile4name = "4 mobile nodes"

# combine stats from all faces
mobile1.combined = summaryBy (. ~ Time + Type, data=data.mobile1, FUN=sum)
mobile2.combined = summaryBy (. ~ Time + Type, data=data.mobile2, FUN=sum)
mobile3.combined = summaryBy (. ~ Time + Type, data=data.mobile3, FUN=sum)
mobile4.combined = summaryBy (. ~ Time + Type, data=data.mobile4, FUN=sum)

mobile1.combined$Variable = "mobile1"
mobile2.combined$Variable = "mobile2"
mobile3.combined$Variable = "mobile3"
mobile4.combined$Variable = "mobile4"

combinedlist = list(mobile1.combined, mobile2.combined, mobile3.combined, mobile4.combined)
allcombineddata = do.call(rbind.fill, combinedlist)

satTime = subset(allcombineddata, Type %in% c("SatisfiedInterests", "TimedOutInterests"))

g.test = ggplot (satTime, aes(linetype=Type, colour=Variable, shape=Type)) +
  geom_line(aes (x=Time, y=PacketRaw.sum), size=1.5) +
  xlab("Time") +
  ylab("Rate [Kbits/s]") +
  ggtitle("Throughput throughout NDN smart fowarding simulation") +
  scale_colour_discrete(name = "Throughput of number of Nodes",
                        labels = c(mobile1name, mobile2name, mobile3name, mobile4name))

print (g.test)