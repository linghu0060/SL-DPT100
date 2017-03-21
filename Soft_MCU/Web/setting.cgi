t     <html>
t       <head>
t         <title>SL-DPT100</title>
t         <meta http-equiv="content-type" content="text/html;charset=gb2312">
t         <script language="JavaScript">
t           function load() {
t             var isSelect = document.getElementsByName("dhcp");
t             var i;
t             for(i = 0; i < isSelect.length; i++) {
t               if(isSelect[i].checked == true) {
t                 i = (isSelect[i].value == "N") ? false : true;
t                 break;
t               }
t             }
t             document.getElementsByName("ip")[0].disabled  = i;
t             document.getElementsByName("msk")[0].disabled = i;
t             document.getElementsByName("gw")[0].disabled  = i;
t           }
t           function changeConfirm(f) {
t             if(!confirm('Are you sure you want to change\nthe Network settings?')) return;
t             f.submit();
t           }
t         </script>
t       </head>
#
i pg_header.inc
t       <body onload="load()">
t       <form action="setting.cgi" method="get" name="cgi">
t         <input type="hidden" value="setting" name="pg">
#
t         <table border="0" width="99%" style="font-size:80%"><tr height="10"></tr></table>    
t         <table border="0" width="99%" style="font-size:80%">
t           <tr bgcolor="#aaccff"><th colSpan="2" align="left"><img src="pabb.gif">ProfiBUS-DP Config</th></tr>
t           <tr><td bgcolor="#EAF2D3" align="right" noWrap width="40%">Baud rate:&nbsp;&nbsp</td>       <!-- bdr --->
c a b           <td><input type="text" name="bdr" id="bdr" value="%u" size="20" maxlength="18" list="baudrate">
t                 <datalist id="baudrate">
t                   <option value="9600"    label="9.6K" />
t                   <option value="19200"   label="19.2K" />
t                   <option value="45450"   label="45.45K" />
t                   <option value="93750"   label="93.75K" />
t                   <option value="187500"  label="187.5K" />
t                   <option value="500000"  label="500K" />
t                   <option value="1500000" label="1.5M" />
t                 </datalist>
t               </td>
t           </tr>
t           <tr height="10"></tr>                                                               
#
t           <tr bgcolor="#aaccff"><th colSpan="2" align="left"><img src="pabb.gif">Ethernet Network Config</th></tr>
t           <tr><td bgcolor="#EAF2D3" align="right" noWrap width="40%">DHCP:&nbsp;&nbsp</td>            <!-- dhcp -->
t               <td><input type="radio" name="dhcp" id="dhcp" checked value="Y" onchange="load()">Enable &nbsp
c a d               <input type="radio" name="dhcp" id="dhcp" %s      value="N" onchange="load()">Disable
t               </td>
t           </tr>
t           <tr><td bgcolor="#EAF2D3" align="right" noWrap width="40%">IP Address:&nbsp;&nbsp</td>      <!-- ip ---->
c a i           <td><input type="text" name="ip" id="ip" value="%s" size="20" maxlength="18"></td>
t           </tr>
t           <tr><td bgcolor="#EAF2D3" align="right" noWrap width="40%">Network Mask:&nbsp;&nbsp</td>    <!-- msk --->
c a m           <td><input type="text" name="msk" id="msk" value="%s" size="20" maxlength="18"></td>
t           </tr>
t           <tr><td bgcolor="#EAF2D3" align="right" noWrap width="40%">Default Gateway:&nbsp;&nbsp</td> <!-- gw ---->
c a g           <td><input type="text" name="gw" id="gw" value="%s" size="20" maxlength="18"></td>
t           </tr>
t           <tr><td bgcolor="#EAF2D3" align="right" noWrap width="40%">MAC Address:&nbsp;&nbsp</td>     <!-- mac --->
c a a           <td><input type="text" name="mac" id="mac" value="%s" size="20" maxlength="18"></td>
t           </tr>
t           <tr height="10"></tr>                                                              
#
t           <tr bgcolor="#aaccff"><th colSpan="2" align="left"><img src="pabb.gif">System Config</th></tr>
t           <tr><td bgcolor="#EAF2D3" align="right" noWrap width="40%">Password:&nbsp;&nbsp</td>        <!-- psw --->
c a w           <td><input type="password" name="psw" id="psw" value="%s" size="20" maxlength="18"></td>
t           </tr>
t         </table>
t         <table border="0" width="99%" style="font-size:80%"><tr height="10"></tr></table>      
#
t         <p align="center">
t           <input type="button" name="set" value="Change" onclick="changeConfirm(this.form)">
t           <input type="reset" value="Undo">
t         </p>
t       </form>
t       </body>
i pg_footer.inc
#
t     </html>
. End of script must be closed with period. -->

