<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:fo="http://www.w3.org/1999/XSL/Format">
	<xsl:template match="af_document">
		<html>
			<HEAD/>
		</html>
		<body>
			<xsl:apply-templates/>
		</body>
	</xsl:template>
	<xsl:template match="text">
		<xsl:for-each select="paragraph">
			<p>
				<xsl:for-each select="sent">
					<a><xsl:attribute name="name">
							<xsl:value-of select="@href_name"/>
						</xsl:attribute>
						<xsl:for-each select="word"><span>
								<xsl:attribute name="title">
									<xsl:for-each select="homonym">
										<xsl:value-of select="@hom_str"/>
										<xsl:if test="position()!=last()">
												<xsl:text >&#10;</xsl:text>
										</xsl:if>
									</xsl:for-each>
								</xsl:attribute>
								<xsl:choose>
									<xsl:when test="@Fio">
										<b>
											<xsl:value-of select="@word_str"/>
										</b>
									</xsl:when>
									<xsl:when test="@CompanyAbbrev">
										<font color="#0000FF">
											<xsl:value-of select="@word_str"/>
										</font>
									</xsl:when>
									<xsl:when test="@SubCompanyDescr">
										<font color="#0088FF">
											<xsl:value-of select="@word_str"/>
										</font>
									</xsl:when>
									<xsl:when test="@CompanyDescr">
										<font color="#00AAAA">
											<xsl:value-of select="@word_str"/>
										</font>
									</xsl:when>
									<xsl:when test="@Company">
										<b>
											<font color="#00AAAA">
												<xsl:value-of select="@word_str"/>
											</font>
										</b>
									</xsl:when>
									<xsl:when test="@CompanyShortName">
										<b>
											<font color="#AAAA00">
												<xsl:value-of select="@word_str"/>
											</font>
										</b>
									</xsl:when>
									<xsl:when test="@SubPost">
										<font color="#AA00AA">
											<xsl:value-of select="@word_str"/>
										</font>
									</xsl:when>
									<xsl:when test="@Post">
										<b>
											<font color="#AA00AA">
												<xsl:value-of select="@word_str"/>
											</font>
										</b>
									</xsl:when>
									<xsl:when test="@Geo">
										<font color="#AA0000">
											<xsl:value-of select="@word_str"/>
										</font>
									</xsl:when>
									<xsl:when test="@Status">
										<font color="#AA0000">
											<xsl:value-of select="@word_str"/>
										</font>
									</xsl:when>	
									<xsl:when test="@Date">
										<font color="#AA0000">
											<xsl:value-of select="@word_str"/>
										</font>
									</xsl:when>																		
									<xsl:otherwise>
										<xsl:value-of select="@word_str"/>
									</xsl:otherwise>
								</xsl:choose>
							</span> 
							&#160;
						</xsl:for-each>
						<b> EOS</b>&#160;</a>
						<xsl:for-each select="situation"> 							
							<table   border="1"  >
								<tbody>
									<tr  align="center">
										<td colspan="10">
											<font color="#0000FF">
												<b>
													<xsl:value-of select="@sit_name"/>
												</b>
											</font>											
										</td>
									</tr>
									<tr align="center" >										
										<xsl:for-each select="val"> 							
											<th>
												<b>
													<xsl:value-of select="@val_name"/>
												</b>
											</th>
										</xsl:for-each>										
									</tr>
									<tr align="center" >										
										<xsl:for-each select="val "> 							
											<td>
												<xsl:value-of select="@val_value"/>
											</td>
										</xsl:for-each>										
									</tr>
								</tbody>
							</table>
						</xsl:for-each>
						<p/>
				</xsl:for-each>
			</p>
		</xsl:for-each>
	</xsl:template>
	
	<xsl:template  match="FactTable">
		<table   border="1"  >
			<tbody>
					<tr align="center">
						<td colspan="10">
							<font color="#0000FF">
								<b>
									<xsl:value-of select="@name"/>
								</b>
							</font>											
						</td>
					</tr>
					<tr align="center" >										
						<xsl:for-each select="FactTableHeader"> 							
							<xsl:for-each select="Cell"> 							
								<th>
									<b>
										<xsl:value-of select="@val"/>
									</b>
								</th>
							</xsl:for-each>										
						</xsl:for-each>										
					</tr>					
					<xsl:for-each select="FactRaw"> 							
						<tr align="center" >										
							<xsl:for-each select="Cell"> 						
								<xsl:choose>									
									<xsl:when test="@val">
										<xsl:choose>
											<xsl:when test="@href">
												<td>
													<a>
														<xsl:attribute name="href">
															<xsl:value-of select="@href"/>
														</xsl:attribute>
														<xsl:value-of select="@val"/>
													</a>
												</td>
											</xsl:when>
											<xsl:otherwise>
												<td><xsl:value-of select="@val"/></td>
											</xsl:otherwise>
										</xsl:choose>		
									</xsl:when>									
									<xsl:otherwise>
										<td>&#160;</td>
									</xsl:otherwise>
																								
								</xsl:choose>						
							</xsl:for-each>										
						</tr>
					</xsl:for-each>															
			</tbody>
		</table>

	</xsl:template>
	
	<xsl:template  match="FPO_objects">
		<table   border="1">
			<tbody>
				<tr align="center">
				    <th>Text</th>
					<th>Type</th>
				</tr>
				<xsl:for-each select="ws">
					<tr align="center">
						<xsl:choose>
							<xsl:when test="@ws_text">
								<td>
									<a>
										<xsl:attribute name="href">
											<xsl:value-of select="@href"/>
										</xsl:attribute>
										<xsl:value-of select="@ws_text"/>
									</a>
									
								</td>
							</xsl:when>
							<xsl:otherwise>
								<td>&#160;</td>
							</xsl:otherwise>
						</xsl:choose>
						<xsl:choose>
							<xsl:when test="@ws_type">
								<td>
									<xsl:value-of select="@ws_type"/>
								</td>
							</xsl:when>
							<xsl:otherwise>
								<td>&#160;</td>
							</xsl:otherwise>
						</xsl:choose>						
					</tr>
				</xsl:for-each>
			</tbody>
		</table>
	</xsl:template>
</xsl:stylesheet>
