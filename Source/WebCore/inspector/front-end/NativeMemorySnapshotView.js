/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @extends {WebInspector.View}
 * @param {WebInspector.NativeMemoryProfileHeader} profile
 */
WebInspector.NativeMemorySnapshotView = function(profile)
{
    WebInspector.View.call(this);
    this.registerRequiredCSS("nativeMemoryProfiler.css");

    this.element.addStyleClass("native-snapshot-view");
    this.containmentDataGrid = new WebInspector.NativeSnapshotDataGrid(profile._memoryBlock);
    this.containmentDataGrid.show(this.element);
}

WebInspector.NativeMemorySnapshotView.prototype = {
    __proto__: WebInspector.View.prototype
}

/**
 * @constructor
 * @extends {WebInspector.DataGrid}
 * @param {MemoryAgent.MemoryBlock} profile
 */
WebInspector.NativeSnapshotDataGrid = function(profile)
{
    var columns = {
        object: { title: WebInspector.UIString("Object"), width: "200px", disclosure: true, sortable: false },
        size: { title: WebInspector.UIString("Size"), sortable: false },
    };
    WebInspector.DataGrid.call(this, columns);
    var totalNode = new WebInspector.NativeSnapshotNode(profile, profile);
    if (WebInspector.settings.showNativeSnapshotUninstrumentedSize.get()) {
        this.setRootNode(new WebInspector.DataGridNode(null, true));
        this.rootNode().appendChild(totalNode)
        totalNode.expand();
    } else {
        this.setRootNode(totalNode);
        totalNode._populate();
    }
}

WebInspector.NativeSnapshotDataGrid.prototype = {
    __proto__: WebInspector.DataGrid.prototype
}

/**
 * @constructor
 * @extends {WebInspector.DataGridNode}
 * @param {MemoryAgent.MemoryBlock} nodeData
 * @param {MemoryAgent.MemoryBlock} profile
 */
WebInspector.NativeSnapshotNode = function(nodeData, profile)
{
    this._nodeData = nodeData;
    this._profile = profile;
    var viewProperties = WebInspector.MemoryBlockViewProperties._forMemoryBlock(nodeData);
    var data = { object: viewProperties._description, size: this._nodeData.size };
    var hasChildren = !!nodeData.children && nodeData.children.length !== 0;
    WebInspector.DataGridNode.call(this, data, hasChildren);
    this.addEventListener("populate", this._populate, this);
}

WebInspector.NativeSnapshotNode.prototype = {
    /**
     * @override
     * @param {string} columnIdentifier
     * @return {Element}
     */
    createCell: function(columnIdentifier)
    {
        var cell = columnIdentifier === "size" ?
            this._createSizeCell(columnIdentifier) :
            WebInspector.DataGridNode.prototype.createCell.call(this, columnIdentifier);
        return cell;
    },

    /**
     * @param {string} columnIdentifier
     * @return {Element}
     */
    _createSizeCell: function(columnIdentifier)
    {
        var node = this;
        var viewProperties = null;
        var dimmed = false;
        while (!viewProperties || viewProperties._fillStyle === "inherit") {
            viewProperties = WebInspector.MemoryBlockViewProperties._forMemoryBlock(node._nodeData);
            if (viewProperties._fillStyle === "inherit")
                dimmed = true;
            node = node.parent;
        }

        var sizeKiB = this._nodeData.size / 1024;
        var totalSize = this._profile.size;
        var percentage = this._nodeData.size / totalSize  * 100;

        var cell = document.createElement("td");
        cell.className = columnIdentifier + "-column";

        var textDiv = document.createElement("div");
        textDiv.textContent = Number.withThousandsSeparator(sizeKiB.toFixed(0)) + "\u2009" + WebInspector.UIString("KiB");
        textDiv.className = "size-text";
        cell.appendChild(textDiv);

        var barDiv = document.createElement("div");
        barDiv.className = "size-bar";
        barDiv.style.width = percentage + "%";
        barDiv.style.backgroundColor = viewProperties._fillStyle;
        // fillerDiv displaces percentage text out of the bar visible area if it doesn't fit.
        var fillerDiv = document.createElement("div");
        fillerDiv.className = "percent-text"
        barDiv.appendChild(fillerDiv);
        var percentDiv = document.createElement("div");
        percentDiv.textContent = percentage.toFixed(1) + "%";
        percentDiv.className = "percent-text"
        barDiv.appendChild(percentDiv);

        var barHolderDiv = document.createElement("div");
        if (dimmed)
            barHolderDiv.className = "dimmed";
        barHolderDiv.appendChild(barDiv);
        cell.appendChild(barHolderDiv);

        return cell;
    },

    _populate: function() {
        this.removeEventListener("populate", this._populate, this);
        function comparator(a, b) {
            return b.size - a.size;
        }
        if (this._nodeData !== this._profile)
            this._nodeData.children.sort(comparator);
        for (var node in this._nodeData.children) {
            var nodeData = this._nodeData.children[node];
            if (WebInspector.settings.showNativeSnapshotUninstrumentedSize.get() || nodeData.name !== "Other")
                this.appendChild(new WebInspector.NativeSnapshotNode(nodeData, this._profile));
        }
    },

    __proto__: WebInspector.DataGridNode.prototype
}

/**
 * @constructor
 * @extends {WebInspector.ProfileType}
 */
WebInspector.NativeMemoryProfileType = function()
{
    WebInspector.ProfileType.call(this, WebInspector.NativeMemoryProfileType.TypeId, WebInspector.UIString("Take Native Memory Snapshot"));
    this._nextProfileUid = 1;
}

WebInspector.NativeMemoryProfileType.TypeId = "NATIVE_MEMORY";

WebInspector.NativeMemoryProfileType.prototype = {
    get buttonTooltip()
    {
        return WebInspector.UIString("Take native memory snapshot.");
    },

    /**
     * @override
     * @param {WebInspector.ProfilesPanel} profilesPanel
     * @return {boolean}
     */
    buttonClicked: function(profilesPanel)
    {
        var profileHeader = new WebInspector.NativeMemoryProfileHeader(this, WebInspector.UIString("Snapshot %d", this._nextProfileUid), this._nextProfileUid);
        ++this._nextProfileUid;
        profileHeader.isTemporary = true;
        profilesPanel.addProfileHeader(profileHeader);
        function didReceiveMemorySnapshot(error, memoryBlock)
        {
            if (memoryBlock.size && memoryBlock.children) {
                var knownSize = 0;
                for (var i = 0; i < memoryBlock.children.length; i++) {
                    var size = memoryBlock.children[i].size;
                    if (size)
                        knownSize += size;
                }
                var otherSize = memoryBlock.size - knownSize;

                if (otherSize) {
                    memoryBlock.children.push({
                        name: "Other",
                        size: otherSize
                    });
                }
            }
            profileHeader._memoryBlock = memoryBlock;
            profileHeader.isTemporary = false;
            profileHeader.sidebarElement.subtitle = Number.bytesToString(memoryBlock.size);
        }
        MemoryAgent.getProcessMemoryDistribution(didReceiveMemorySnapshot.bind(this));
        return false;
    },

    get treeItemTitle()
    {
        return WebInspector.UIString("MEMORY DISTRIBUTION");
    },

    get description()
    {
        return WebInspector.UIString("Native memory snapshot profiles show memory distribution among browser subsystems");
    },

    /**
     * @override
     * @param {string=} title
     * @return {WebInspector.ProfileHeader}
     */
    createTemporaryProfile: function(title)
    {
        title = title || WebInspector.UIString("Snapshotting\u2026");
        return new WebInspector.NativeMemoryProfileHeader(this, title);
    },

    /**
     * @override
     * @param {ProfilerAgent.ProfileHeader} profile
     * @return {WebInspector.ProfileHeader}
     */
    createProfile: function(profile)
    {
        return new WebInspector.NativeMemoryProfileHeader(this, profile.title, -1);
    },

    __proto__: WebInspector.ProfileType.prototype
}

/**
 * @constructor
 * @extends {WebInspector.ProfileHeader}
 * @param {!WebInspector.NativeMemoryProfileType} type
 * @param {string} title
 * @param {number=} uid
 */
WebInspector.NativeMemoryProfileHeader = function(type, title, uid)
{
    WebInspector.ProfileHeader.call(this, type, title, uid);

    /**
     * @type {MemoryAgent.MemoryBlock}
     */
    this._memoryBlock = null;
}

WebInspector.NativeMemoryProfileHeader.prototype = {
    /**
     * @override
     */
    createSidebarTreeElement: function()
    {
        return new WebInspector.ProfileSidebarTreeElement(this, WebInspector.UIString("Snapshot %d"), "heap-snapshot-sidebar-tree-item");
    },

    /**
     * @override
     * @param {WebInspector.ProfilesPanel} profilesPanel
     */
    createView: function(profilesPanel)
    {
        return new WebInspector.NativeMemorySnapshotView(this);
    },

    __proto__: WebInspector.ProfileHeader.prototype
}

/**
 * @constructor
 * @param {string} fillStyle
 * @param {string} name
 * @param {string} description
 */
WebInspector.MemoryBlockViewProperties = function(fillStyle, name, description)
{
    this._fillStyle = fillStyle;
    this._name = name;
    this._description = description;
}

/**
 * @type {Object.<string, WebInspector.MemoryBlockViewProperties>}
 */
WebInspector.MemoryBlockViewProperties._standardBlocks = null;

WebInspector.MemoryBlockViewProperties._initialize = function()
{
    if (WebInspector.MemoryBlockViewProperties._standardBlocks)
        return;
    WebInspector.MemoryBlockViewProperties._standardBlocks = {};
    function addBlock(fillStyle, name, description)
    {
        WebInspector.MemoryBlockViewProperties._standardBlocks[name] = new WebInspector.MemoryBlockViewProperties(fillStyle, name, WebInspector.UIString(description));
    }
    addBlock("hsl(  0,  0%,  60%)", "ProcessPrivateMemory", "Total");
    addBlock("hsl(  0,  0%,  80%)", "OwnersTypePlaceholder", "OwnersTypePlaceholder");
    addBlock("hsl(  0,  0%,  60%)", "Other", "Other");
    addBlock("hsl(220, 80%,  70%)", "Page", "Page structures");
    addBlock("hsl(100, 60%,  50%)", "JSHeap", "JavaScript heap");
    addBlock("hsl( 90, 40%,  80%)", "JSExternalResources", "JavaScript external resources");
    addBlock("hsl( 90, 60%,  80%)", "JSExternalArrays", "JavaScript external arrays");
    addBlock("hsl( 90, 60%,  80%)", "JSExternalStrings", "JavaScript external strings");
    addBlock("hsl(  0, 80%,  60%)", "WebInspector", "Inspector data");
    addBlock("hsl( 36, 90%,  50%)", "MemoryCache", "Memory cache resources");
    addBlock("hsl( 40, 80%,  80%)", "GlyphCache", "Glyph cache resources");
    addBlock("hsl( 35, 80%,  80%)", "DOMStorageCache", "DOM storage cache");
    addBlock("hsl( 60, 80%,  60%)", "RenderTree", "Render tree");
}

WebInspector.MemoryBlockViewProperties._forMemoryBlock = function(memoryBlock)
{
    WebInspector.MemoryBlockViewProperties._initialize();
    var result = WebInspector.MemoryBlockViewProperties._standardBlocks[memoryBlock.name];
    if (result)
        return result;
    return new WebInspector.MemoryBlockViewProperties("inherit", memoryBlock.name, memoryBlock.name);
}


/**
 * @constructor
 * @extends {WebInspector.View}
 * @param {MemoryAgent.MemoryBlock} memorySnapshot
 */
WebInspector.NativeMemoryPieChart = function(memorySnapshot)
{
    WebInspector.View.call(this);
    this._memorySnapshot = memorySnapshot;
    this.element = document.createElement("div");
    this.element.addStyleClass("memory-pie-chart-container");
    this._memoryBlockList = this.element.createChild("div", "memory-blocks-list");

    this._canvasContainer = this.element.createChild("div", "memory-pie-chart");
    this._canvas = this._canvasContainer.createChild("canvas");
    this._addBlockLabels(memorySnapshot, true);
}

WebInspector.NativeMemoryPieChart.prototype = {
    /**
     * @override
     */
    onResize: function()
    {
        this._updateSize();
        this._paint();
    },

    _updateSize: function()
    {
        var width = this._canvasContainer.clientWidth - 5;
        var height = this._canvasContainer.clientHeight - 5;
        this._canvas.width = width;
        this._canvas.height = height;
    },

    _addBlockLabels: function(memoryBlock, includeChildren)
    {
        var viewProperties = WebInspector.MemoryBlockViewProperties._forMemoryBlock(memoryBlock);
        var title = viewProperties._description + ": " + Number.bytesToString(memoryBlock.size);

        var swatchElement = this._memoryBlockList.createChild("div", "item");
        swatchElement.createChild("div", "swatch").style.backgroundColor = viewProperties._fillStyle;
        swatchElement.createChild("span", "title").textContent = title;

        if (!memoryBlock.children || !includeChildren)
            return;
        for (var i = 0; i < memoryBlock.children.length; i++)
            this._addBlockLabels(memoryBlock.children[i], false);
    },

    _paint: function()
    {
        this._clear();
        var width = this._canvas.width;
        var height = this._canvas.height;

        var x = width / 2;
        var y = height / 2;
        var radius = 200;

        var ctx = this._canvas.getContext("2d");
        ctx.beginPath();
        ctx.arc(x, y, radius, 0, Math.PI*2, false);
        ctx.lineWidth = 1;
        ctx.strokeStyle = "rgba(130, 130, 130, 0.8)";
        ctx.stroke();
        ctx.closePath();

        var currentAngle = 0;
        var memoryBlock = this._memorySnapshot;

        function paintPercentAndLabel(fraction, title, midAngle)
        {
            ctx.beginPath();
            ctx.font = "13px Arial";
            ctx.fillStyle = "rgba(10, 10, 10, 0.8)";

            var textX = x + (radius + 10) * Math.cos(midAngle);
            var textY = y + (radius + 10) * Math.sin(midAngle);
            var relativeOffset = -Math.cos(midAngle) / Math.sin(Math.PI / 12);
            relativeOffset = Number.constrain(relativeOffset, -1, 1);
            var metrics = ctx.measureText(title);
            textX -= metrics.width * (relativeOffset + 1) / 2;
            textY += 5;
            ctx.fillText(title, textX, textY);

            // Do not print percentage if the sector is too narrow.
            if (fraction > 0.03) {
                textX = x + radius * Math.cos(midAngle) / 2;
                textY = y + radius * Math.sin(midAngle) / 2;
                ctx.fillText((100 * fraction).toFixed(0) + "%", textX - 8, textY + 5);
            }

            ctx.closePath();
        }

        if (!memoryBlock.children)
            return;
        var total = memoryBlock.size;
        for (var i = 0; i < memoryBlock.children.length; i++) {
            var child = memoryBlock.children[i];
            if (!child.size)
                continue;
            var viewProperties = WebInspector.MemoryBlockViewProperties._forMemoryBlock(child);
            var angleSpan = Math.PI * 2 * (child.size / total);
            ctx.beginPath();
            ctx.moveTo(x, y);
            ctx.lineTo(x + radius * Math.cos(currentAngle), y + radius * Math.sin(currentAngle));
            ctx.arc(x, y, radius, currentAngle, currentAngle + angleSpan, false);
            ctx.lineWidth = 0.5;
            ctx.lineTo(x, y);
            ctx.fillStyle = viewProperties._fillStyle;
            ctx.strokeStyle = "rgba(100, 100, 100, 0.8)";
            ctx.fill();
            ctx.stroke();
            ctx.closePath();

            paintPercentAndLabel(child.size / total, viewProperties._description, currentAngle + angleSpan / 2);

            currentAngle += angleSpan;
        }
    },

    _clear: function() {
        var ctx = this._canvas.getContext("2d");
        ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
    },

    __proto__: WebInspector.View.prototype
}

/**
 * @constructor
 * @extends {WebInspector.View}
 */
WebInspector.NativeMemoryBarChart = function()
{
    WebInspector.View.call(this);
    this.registerRequiredCSS("nativeMemoryProfiler.css");
    this._memorySnapshot = null;
    this.element = document.createElement("div");
    this._table = this.element.createChild("table");
    this._divs = {};
    var row = this._table.insertRow();
    this._totalDiv = row.insertCell().createChild("div");
    this._totalDiv.addStyleClass("memory-bar-chart-total");
    row.insertCell();
}

WebInspector.NativeMemoryBarChart.prototype = {
    _updateStats: function()
    {
        function didReceiveMemorySnapshot(error, memoryBlock)
        {
            if (memoryBlock.size && memoryBlock.children) {
                var knownSize = 0;
                for (var i = 0; i < memoryBlock.children.length; i++) {
                    var size = memoryBlock.children[i].size;
                    if (size)
                        knownSize += size;
                }
                var otherSize = memoryBlock.size - knownSize;

                if (otherSize) {
                    memoryBlock.children.push({
                        name: "Other",
                        size: otherSize
                    });
                }
            }
            this._memorySnapshot = memoryBlock;
            this._updateView();
        }
        MemoryAgent.getProcessMemoryDistribution(didReceiveMemorySnapshot.bind(this));
    },

    /**
     * @override
     */
    willHide: function()
    {
        clearInterval(this._timerId);
    },

    /**
     * @override
     */
    wasShown: function()
    {
        this._timerId = setInterval(this._updateStats.bind(this), 1000);
    },

    _updateView: function()
    {
        var memoryBlock = this._memorySnapshot;
        if (!memoryBlock)
            return;

        var MB = 1024 * 1024;
        var maxSize = 100 * MB;
        for (var i = 0; i < memoryBlock.children.length; ++i)
            maxSize = Math.max(maxSize, memoryBlock.children[i].size);
        var maxBarLength = 500;
        var barLengthSizeRatio = maxBarLength / maxSize;

        for (var i = memoryBlock.children.length - 1; i >= 0 ; --i) {
            var child = memoryBlock.children[i];
            var name = child.name;
            var divs = this._divs[name];
            if (!divs) {
                var row = this._table.insertRow();
                var nameDiv = row.insertCell(-1).createChild("div");
                var viewProperties = WebInspector.MemoryBlockViewProperties._forMemoryBlock(child);
                var title = viewProperties._description;
                nameDiv.textContent = title;
                nameDiv.addStyleClass("memory-bar-chart-name");
                var barCell = row.insertCell(-1);
                var barDiv = barCell.createChild("div");
                barDiv.addStyleClass("memory-bar-chart-bar");
                viewProperties = WebInspector.MemoryBlockViewProperties._forMemoryBlock(child);
                barDiv.style.backgroundColor = viewProperties._fillStyle;
                var unusedDiv = barDiv.createChild("div");
                unusedDiv.addStyleClass("memory-bar-chart-unused");
                var percentDiv = barDiv.createChild("div");
                percentDiv.addStyleClass("memory-bar-chart-percent");
                var sizeDiv = barCell.createChild("div");
                sizeDiv.addStyleClass("memory-bar-chart-size");
                divs = this._divs[name] = { barDiv: barDiv, unusedDiv: unusedDiv, percentDiv: percentDiv, sizeDiv: sizeDiv };
            }
            var unusedSize = 0;
            if (!!child.children) {
                var unusedName = name + ".Unused";
                for (var j = 0; j < child.children.length; ++j) {
                    if (child.children[j].name === unusedName) {
                        unusedSize = child.children[j].size;
                        break;
                    }
                }
            }
            var unusedLength = unusedSize * barLengthSizeRatio;
            var barLength = child.size * barLengthSizeRatio;

            divs.barDiv.style.width = barLength + "px";
            divs.unusedDiv.style.width = unusedLength + "px";
            divs.percentDiv.textContent = barLength > 20 ? (child.size / memoryBlock.size * 100).toFixed(0) + "%" : "";
            divs.sizeDiv.textContent = (child.size / MB).toFixed(1) + "\u2009MB";
        }

        var memoryBlockViewProperties = WebInspector.MemoryBlockViewProperties._forMemoryBlock(memoryBlock);
        this._totalDiv.textContent = memoryBlockViewProperties._description + ": " + (memoryBlock.size / MB).toFixed(1) + "\u2009MB";
    },

    __proto__: WebInspector.View.prototype
}
