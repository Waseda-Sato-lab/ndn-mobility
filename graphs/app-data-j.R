#!/usr/bin/Rscript

# app-data-j.R
# Simple R script to make graphs from ndnSIM tracers - App delay
#
# Copyright (c) 2014 Waseda University, Sato Laboratory
# Author: Jairo Eduardo Lopez <jairo@ruri.waseda.jp>
#
# app-data-j.R is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# app-data-j.R is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of              
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               
# GNU Affero Public License for more details.                                 
#                                                                             
# You should have received a copy of the GNU Affero Public License            
# along with app-data-j.R.  If not, see <http://www.gnu.org/licenses/>.
#                    

suppressPackageStartupMessages(library (ggplot2))
suppressPackageStartupMessages(library (scales))
suppressPackageStartupMessages(library (optparse))
suppressPackageStartupMessages(library (doBy))

# set some reasonable defaults for the options that are needed
option_list <- list (
  make_option(c("-f", "--file"), type="character", default="results/app-delay.txt",
              help="File which holds the raw app delay data.\n\t\t[Default \"%default\"]"),
  make_option(c("-d", "--delay"), action="store_true", default=FALSE,
              help="Tell the script to graph delay data"),
  make_option(c("-j", "--hop"), action="store_true", default=FALSE,
              help="Tell the script to graph hop data"),
  make_option(c("-o", "--output"), type="character", default=".",
              help="Output directory for graphs.\n\t\t[Default \"%default\"]"),
  make_option(c("-e", "--node"), type="character", default="",
              help="Node data to graph. Can be a comma separated list.\n\t\tDefault graphs data for all nodes."),
  make_option(c("-t", "--title"), type="character", default="NDN App",
              help="Title for the graph")
)

# Load the parser
opt = parse_args(OptionParser(option_list=option_list, description="Creates graphs from ndnSIM App Delay Tracer data"))

data = read.table (opt$file, header=T)
data$Node = factor (data$Node)
data$Type = factor (data$Type)

data$TimeSec = 1 * ceiling (data$Time)

# exclude irrelevant types - CCN
data = subset (data, Type %in% c("FullDelay"))

filnodes = unlist(strsplit(opt$node, ","))

# Filter for a particular node
if (nchar(opt$node) > 0) {
  
  data = subset (data, Node %in% filnodes)
}

# Get the basename of the file
tmpname = strsplit(opt$file, "/")[[1]]
filename = tmpname[length(tmpname)]
# Get rid of the extension
noext = gsub("\\..*", "", filename)

if (opt$delay) {
  name = sprintf("%s Average Network Delay", opt$title)
  
  data.combined = summaryBy (. ~ TimeSec + Type, data=data, FUN=mean)
  g.all <- ggplot (data.combined, aes(colour=Legend)) +
    geom_line (aes (x=TimeSec, y=DelayS.mean, colour="Avg Delay"), size=1) +
    ggtitle (name) +
    ylab ("Delay [Seconds]") +
    xlab ("Time")
  
  outpng = sprintf("%s/%s-app-delay.png", opt$output, noext)
  
  png (outpng, width=1024, height=768)
  print (g.all)
  x = dev.off ()
}

if (opt$hop) {
  name = sprintf("%s Average Packet Hop Count", opt$title)
  
  data.combined = summaryBy (. ~ TimeSec + Type, data=data, FUN=mean)
  
  g.all <- ggplot (data.combined, aes(colour=Legend)) +
    geom_line (aes (x=TimeSec, y=HopCount.mean, colour="Avg Hops"), size=1) +
    ggtitle (name) +
    ylab ("Hop Count") +
    xlab ("Time")
  
  outpng = sprintf("%s/%s-app-hop.png", opt$output, noext)
  
  png (outpng, width=1024, height=768)
  print (g.all)
  x = dev.off ()
}