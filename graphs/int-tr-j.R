#!/usr/bin/Rscript

# int-tr-j.R
# Simple R script to make graphs from ndnSIM tracers - Satisfied Interests / Timed out interests

#
# Copyright (c) 2014 Waseda University, Sato Laboratory
# Author: Jairo Eduardo Lopez <jairo@ruri.waseda.jp>
#
# int-tr-j.R is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# int-tr-j.R is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of              
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               
# GNU Affero Public License for more details.                                 
#                                                                             
# You should have received a copy of the GNU Affero Public License            
# along with int-tr-j.R.  If not, see <http://www.gnu.org/licenses/>.
#

suppressPackageStartupMessages(library (ggplot2))
suppressPackageStartupMessages(library (scales))
suppressPackageStartupMessages(library (optparse))
suppressPackageStartupMessages(library (doBy))

# set some reasonable defaults for the options that are needed
option_list <- list (
  make_option(c("-f", "--file"), type="character", default="results/aggregate-trace.txt",
              help="File which holds the raw aggregate data.\n\t\t[Default \"%default\"]"),
  make_option(c("-e", "--node"), type="character", default="",
              help="Node data to graph. Can be a comma separated list.\n\t\tDefault graphs data for all nodes."),
  make_option(c("-o", "--output"), type="character", default=".",
              help="Output directory for graphs.\n\t\t[Default \"%default\"]"),
  make_option(c("-t", "--title"), type="character", default="Scenario",
              help="Title for the graph")
)

# Load the parser
opt = parse_args(OptionParser(option_list=option_list, description="Creates graphs from ndnSIM L3 Data Aggregate Tracer data"))

data = read.table (opt$file, header=T)
data$Node = factor (data$Node)
data$Kilobits <- data$Kilobytes * 8
data$Type = factor (data$Type)

intdata = data

intdata = subset(intdata, Type %in% c("SatisfiedInterests", "TimedOutInterests"))

name = ""
filnodes = unlist(strsplit(opt$node, ","))

# Get the basename of the file
tmpname = strsplit(opt$file, "/")[[1]]
filename = tmpname[length(tmpname)]
# Get rid of the extension
noext = gsub("\\..*", "", filename)

# Print the Interest information if the data is from CCN  
intdata.combined = ""
g.int = ""
# Filter for a particular node
if (nchar(opt$node) > 0) {
  intdata = subset (intdata, Node %in% filnodes)
  
  if (dim(data)[1] == 0) {
    cat(sprintf("There is no Node %s in this trace!\n", opt$node))
    quit("yes")
  }
  
  intname = sprintf("%s Interest Success rate for Nodes %s", opt$title, opt$node)
  
  intdata.combined = summaryBy (. ~ Time + Node + Type, data=intdata, FUN=sum)
  
  g.int <- ggplot (intdata.combined, aes(x=Time, y=Packets.sum, color=Type)) +
    geom_line(aes (linetype=Type), size=0.5) + 
    geom_point(aes (shape=Type), size=1) +  
    ggtitle (intname) +
    ylab ("Packets / Seconds") +
    xlab ("Simulation time (Seconds)") +
    facet_wrap (~ Node)
} else {
  intname = sprintf("%s Interest Success rate", opt$title)
  
  intdata.combined = summaryBy (. ~ Time + Type, data=intdata, FUN=sum)
  
  g.int <- ggplot (intdata.combined, aes(x=Time, y=Packets.sum, color=Type)) +
    geom_line(aes (linetype=Type), size=0.5) + 
    geom_point(aes (shape=Type), size=1) +  
    ggtitle (intname) +
    ylab ("Packets / Seconds") +
    xlab ("Simulation time (Seconds)")
}

outInpng = ""
# The output png
if (nchar(opt$node) > 0) {
  outInpng = sprintf("%s/%s-%s-sin.png", opt$output, noext, paste(filnodes, sep="", collapse="_"))
} else {
  outInpng = sprintf("%s/%s-sin.png", opt$output, noext)
}

png (outInpng, width=1024, height=768)
print (g.int)
x = dev.off ()
